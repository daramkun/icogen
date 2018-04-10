using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace Daramee.IconGenerator
{
	/// <summary>
	/// MainWindow.xaml에 대한 상호 작용 논리
	/// </summary>
	public partial class MainWindow : Window
	{
		int [] sizeList = new int [] { 256, 192, 128, 96, 64, 48, 32, 16 };
		Dictionary<int, CheckBox> checkBoxDictionary;
		Dictionary<int, Image> imageDictionary;
		BitmapSource nullImage = new WriteableBitmap ( 256, 256, 96, 96, PixelFormats.Pbgra32, null );

		public MainWindow ()
		{
			InitializeComponent ();

			checkBoxDictionary = new Dictionary<int, CheckBox> ()
			{
				{ 256, checkBox256 },
				{ 192, checkBox192 },
				{ 128, checkBox128 },
				{ 96, checkBox96 },
				{ 64, checkBox64 },
				{ 48, checkBox48 },
				{ 32, checkBox32 },
				{ 16, checkBox16 },
			};
			imageDictionary = new Dictionary<int, Image> ()
			{
				{ 256, image256 },
				{ 192, image192 },
				{ 128, image128 },
				{ 96, image96 },
				{ 64, image64 },
				{ 48, image48 },
				{ 32, image32 },
				{ 16, image16 },
			};

			image256.Source = image192.Source = image128.Source = image96.Source = image64.Source
				= image48.Source = image32.Source = image16.Source = nullImage;
		}

		private void SourcePathBrowse_Click ( object sender, RoutedEventArgs e )
		{
			OpenFileDialog ofd = new OpenFileDialog ()
			{
				Filter = "이미지 파일(*.bmp;*.png;*.gif;*.jpg;*.jpeg;*.tif;*.tiff;*.ico)|*.bmp;*.png;*.gif;*.jpg;*.jpeg;*.tif;*.tiff;*.ico",
			};
			if ( ofd.ShowDialog () == false )
				return;

			if ( File.Exists ( ofd.FileName ) )
			{
				BitmapImage imageSource = new BitmapImage ();
				imageSource.BeginInit ();
				imageSource.UriSource = new Uri ( ofd.FileName );
				imageSource.EndInit ();
				imageSource.Freeze ();

				BitmapSource bitmapSource = imageSource;
				if ( imageSource.PixelWidth != imageSource.PixelHeight )
				{
					DrawingVisual drawingVisual = new DrawingVisual ();
					using ( DrawingContext drawingContext = drawingVisual.RenderOpen () )
					{
						drawingContext.DrawRectangle (
							new SolidColorBrush ( Colors.Transparent ),
							null, new Rect ( 0, 0, 256, 256 ) );
						Rect resizeFactor;
						if ( imageSource.PixelWidth > imageSource.PixelHeight )
						{
							resizeFactor = new Rect ( 0, 0, 256, 256 * imageSource.PixelHeight / imageSource.PixelWidth );
							resizeFactor.Y = 256 / 2 - resizeFactor.Height / 2;
						}
						else
						{
							resizeFactor = new Rect ( 0, 0, 256 * imageSource.PixelWidth / imageSource.PixelHeight, 256 );
							resizeFactor.X = 256 / 2 - resizeFactor.Width / 2;
						}
						drawingContext.DrawImage ( imageSource, resizeFactor );
					}
					
					RenderTargetBitmap resizedImage = new RenderTargetBitmap (
						256, 256, 96, 96, PixelFormats.Default );
					resizedImage.Render ( drawingVisual );

					bitmapSource = resizedImage;
				}

				image256.Source = image192.Source = image128.Source = image96.Source = image64.Source
					= image48.Source = image32.Source = image16.Source = bitmapSource;
			}
			else
			{
				image256.Source = image192.Source = image128.Source = image96.Source = image64.Source
					= image48.Source = image32.Source = image16.Source = nullImage;
			}
		}
		
		private DrawingVisual ModifyToDrawingVisual ( Visual v, Rect b )
		{
			/// new a drawing visual and get its context
			DrawingVisual dv = new DrawingVisual ();
			DrawingContext dc = dv.RenderOpen ();

			/// generate a visual brush by input, and paint
			VisualBrush vb = new VisualBrush ( v );
			dc.DrawRectangle ( vb, null, b );
			dc.Close ();

			return dv;
		}

		private void SaveButton_Click ( object sender, RoutedEventArgs e )
		{
			SaveFileDialog sfd = new SaveFileDialog ()
			{
				Filter = "아이콘 파일(*.ico)|*.ico",
			};
			if ( sfd.ShowDialog () == false )
				return;

			using ( Stream outputStream = new FileStream ( sfd.FileName, FileMode.Create ) )
			{
				IconEncoder encoder = new IconEncoder ( outputStream );
				foreach ( int size in sizeList.Reverse () )
				{
					if ( checkBoxDictionary [ size ].IsChecked == false )
						continue;

					RenderTargetBitmap rtb = new RenderTargetBitmap ( size, size, 96, 96, PixelFormats.Pbgra32 );
					rtb.Render ( ModifyToDrawingVisual ( imageDictionary [ size ], new Rect ( 0, 0, size, size ) ) );
					rtb.Freeze ();

					encoder.InsertImage ( rtb );
				}
				encoder.Encode ();
			}
		}

		private void CopyFromClipboard_Click ( object sender, RoutedEventArgs e )
		{
			Image image = null;
			if ( sender is MenuItem menu )
				image = ( menu.Parent as ContextMenu ).PlacementTarget as Image;
			if ( image == null ) return;

			if ( Clipboard.ContainsFileDropList () )
			{
				BitmapImage source = new BitmapImage ();
				source.BeginInit ();
				source.UriSource = new Uri ( Clipboard.GetFileDropList () [ 0 ] );
				source.EndInit ();
				source.Freeze ();

				image.Source = source;
			}
			else if ( Clipboard.ContainsImage () )
			{
				BitmapSource source = Clipboard.GetImage ();
				source.Freeze ();

				image.Source = source;
			}
			else if ( Clipboard.ContainsText () )
			{
				string path = Clipboard.GetText ();
				if ( File.Exists ( path ) )
				{
					BitmapImage source = new BitmapImage ();
					source.BeginInit ();
					source.UriSource = new Uri ( path );
					source.EndInit ();
					source.Freeze ();

					image.Source = source;
				}
			}
		}
	}
}
