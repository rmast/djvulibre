#include <iostream>

#include <DjVuDocument.h>
#include <DjVuImage.h>
#include <DjVmDir.h>
#include <JB2Image.h>
#include <Arrays.h>
#include <GBitmap.h>
#include <BSByteStream.h>

using namespace DJVU;

int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cout << "usage: " << argv[0] << " <filename>" << std::endl;
		return -1;
	}

	G_TRY
	{
		const GURL::Filename::UTF8 url(argv[1]);
		GP<DjVuDocument> doc = DjVuDocument::create_wait(url);
		if (!doc) {
			std::cerr << "cannot open file" << std::endl;
			return -1;
		}

		int found = 0;
		const int pages = doc->get_pages_num();
		//const int pages = 1;
		for(int page = 0; page < pages; page++) {
			std::cout << "testing page " << page << "... ";

			GP<DjVuImage> dimg = doc->get_page(page);
			if (!dimg) {
				std::cout << "get_page failed";
			} else {
				GP<JB2Image> jimg = dimg->get_fgjb();
				if (!jimg) {
					std::cout << "get_fgjb failed";
				} else {
					if (jimg->get_inherited_dict()) {
						std::cout <<
							"found inherited dictionary, inherited shapecount = " <<
							jimg->get_inherited_shape_count();

						found++;
					} else {
						std::cout << "inherited dictionary not found";
					}
/*					std::cout << std::endl;
					// test the shapes
					int i = 0;
					int j = 5;

					while (i<j && i<jimg->get_shape_count()) {
						JB2Shape shape = jimg->get_shape(i);
						GP<GBitmap> bits = shape.bits;
						if (!bits)
							continue;
						GP<ByteStream> bs = ByteStream::create();
						bits->save_pbm(*bs);
						//TArray<char> array = bs->get_data();
					    const unsigned char* data = reinterpret_cast<const unsigned char*>((char*) bs->get_data());
					    for (int k=0; k<(int) bs->size();k++) {
					    	std::cout << (int) data[k] << " ";
					    	if ((k+1)%10 == 0) {
					    		std::cout << std::endl;
					    	}
					    }

					    std::cout << std::endl;
						i++;
					}*/
				}
			}
			std::cout << std::endl;

		}

		std::cout <<
			std::endl <<
			"Summary: " <<
			found <<
			" pages with ihherited shapes dictionary found" <<
			std::endl;
	} G_CATCH(ex) {
		ex.perror();
		return 1;
	}
	G_ENDCATCH;

	return 0;
}

