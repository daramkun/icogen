using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace Daramee.IconGenerator
{
	public class IconEncoder : IDisposable
	{
		[StructLayout ( LayoutKind.Sequential, Pack = 1 )]
		struct IconHeader
		{
			public ushort reserved, type, count;
		}

		[StructLayout ( LayoutKind.Sequential, Pack = 1 )]
		struct EntryHeader
		{
			public byte width;
			public byte height;
			public byte colorCount;
			public byte reserved;
			public ushort planes;
			public ushort bitCount;
			public uint sizeInBytes;
			public uint fileOffset;
		}

		Stream outputStream;

		List<KeyValuePair<Stream, EntryHeader>> entries;

		public IconEncoder ( string filename )
			: this ( new FileStream ( filename, FileMode.Create ) )
		{ }
		public IconEncoder ( Stream stream )
		{
			outputStream = stream;
			entries = new List<KeyValuePair<Stream, EntryHeader>> ();
		}

		private Stream EncodeImageToPNG ( BitmapSource imageSource )
		{
			PngBitmapEncoder encoder = new PngBitmapEncoder ();
			encoder.Frames.Add ( BitmapFrame.Create ( imageSource ) );

			MemoryStream stream = new MemoryStream ();
			encoder.Save ( stream );
			stream.Position = 0;

			return stream;
		}

		private void WriteToStream<T> ( Stream stream, T s ) where T : struct
		{
			IntPtr alloc = Marshal.AllocHGlobal ( Marshal.SizeOf<T> () );
			Marshal.StructureToPtr<T> ( s, alloc, false );
			byte [] array = new byte [ Marshal.SizeOf<T> () ];
			Marshal.Copy ( alloc, array, 0, array.Length );
			stream.Write ( array, 0, array.Length );
			Marshal.FreeHGlobal ( alloc );
		}

		private BitmapSource ConvertIndexedColorImage ( BitmapSource source )
		{
			BitmapPalette myPalette = new BitmapPalette ( source, 256 );
			PixelFormat pixelFormat = PixelFormats.Indexed8;
			if ( myPalette.Colors.Count < 2 )
				pixelFormat = PixelFormats.Indexed1;
			else if ( myPalette.Colors.Count < 4 )
				pixelFormat = PixelFormats.Indexed2;
			else if ( myPalette.Colors.Count < 16 )
				pixelFormat = PixelFormats.Indexed4;
			else if ( myPalette.Colors.Count == 256 )
				return source;

			FormatConvertedBitmap newFormatedBitmapSource = new FormatConvertedBitmap ();
			newFormatedBitmapSource.BeginInit ();
			newFormatedBitmapSource.Source = source;
			newFormatedBitmapSource.DestinationPalette = myPalette;
			newFormatedBitmapSource.DestinationFormat = pixelFormat;
			newFormatedBitmapSource.EndInit ();

			return newFormatedBitmapSource;
		}

		public void InsertImage ( BitmapSource imageSource )
		{
			//imageSource = ConvertIndexedColorImage ( imageSource );
			Stream stream = EncodeImageToPNG ( imageSource );

			EntryHeader header = new EntryHeader
			{
				width = ( byte ) ( uint ) imageSource.Width,
				height = ( byte ) ( uint ) imageSource.Height,
				colorCount = ( byte ) ( imageSource.Palette != null ? imageSource.Palette.Colors.Count : 0 ),
				planes = 1,
				bitCount = 0,
				sizeInBytes = ( uint ) stream.Length
			};

			entries.Add ( new KeyValuePair<Stream, EntryHeader> ( stream, header ) );
		}

		public void Encode ()
		{
			IconHeader header = new IconHeader ()
			{
				reserved = 0,
				type = 1,
				count = ( ushort ) entries.Count,
			};
			WriteToStream<IconHeader> ( outputStream, header );

			uint totalWritten = ( uint ) Marshal.SizeOf<IconHeader> ()
				+ ( uint ) entries.Count * ( uint ) Marshal.SizeOf<EntryHeader> ();
			foreach ( var pair in entries )
			{
				var entryHeader = pair.Value;
				entryHeader.fileOffset = totalWritten;
				WriteToStream<EntryHeader> ( outputStream, entryHeader );
				totalWritten += ( uint ) pair.Key.Length;
			}
			byte [] buffer = new byte [ 4194304 ];
			foreach ( var pair in entries )
			{
				var stream = pair.Key;
				int totalCopy = 0;
				while ( totalCopy < stream.Length )
				{
					int read = stream.Read ( buffer, 0, buffer.Length );
					outputStream.Write ( buffer, 0, read );
					totalCopy += read;
				}
			}

			outputStream.Flush ();
		}

		public void Dispose ()
		{
			outputStream.Dispose ();
		}
	}
}
