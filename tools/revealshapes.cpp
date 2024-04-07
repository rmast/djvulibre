/***
 * 
 * Exportshapes is a program that extracts shape dictionary data 
 * from a djvu file into a database.
 *
 * Copyright © 2012 -- Piotr Sikora.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 ***/

#include <iostream>
#include <exception>
#include <string>
#include <sstream>
#include <map>
#include <vector>

#include <mysql.h>

#include <boost/scope_exit.hpp>

#include <DjVuDocument.h>
#include <DjVuImage.h>
#include <DjVmDir.h>
#include <JB2Image.h>
#include <Arrays.h>
#include <GBitmap.h>
#include <UnicodeByteStream.h>

using namespace std;

typedef std::pair<int, int> IntPair;

typedef struct {
	int right, top, left, bottom;
} BoundingBox;

bool test_run = false;


int process_document(int page_from, int page_to, GP<DjVuDocument> doc) {
	const int pages = doc->get_pages_num();
	int page_start, page_limit;
	page_start = (page_from < 1) ? 0 : page_from - 1;
	if (page_to < 1 || page_to >= pages)
		page_limit = pages;
	else
		page_limit = page_to;
	if (page_limit <= page_start) {
		std::cout << "Can't process less than one page." << std::endl;
		return EXIT_FAILURE;
	}


	std::cout << "sjbz or djbz,page number,blit number,blit shapeno,shape bits columns,rows,rowsize,blit bottom, left" << endl;


	for(int page_number = page_start; page_number < page_limit; page_number++) {

		GP<DjVuImage> djvu_page = doc->get_page(page_number);

		if (!djvu_page) {
			std::cout << "Failed to get DjVuImage for page" << page_number << std::endl;
		} else {

			GP<DjVuFile> djvu_file = djvu_page->get_djvu_file();
			GP<JB2Image> jimg = djvu_page->get_fgjb();
			if (!djvu_file) {
				std::cout << "Failed to get DjVUFile for page " << page_number << std::endl;
			} else if (!jimg) {
				std::cout << "Failed to get JB2Image for page " << page_number << std::endl;
			} else {//export shapes
				std::cout << "Processing page " << page_number << " containing " << jimg->get_shape_count() << " shapes, " <<
						jimg->get_inherited_shape_count() << " of them inherited." << std::endl;
				GP<JB2Dict> inherited_dictionary = jimg->get_inherited_dict();
				// int inh_dict_id = -1;
				int inh_sh_count = jimg->get_inherited_shape_count();
				// string dict_name = "";

				// if (inherited_dictionary) {
				// 	if (test_run)
				// 		std::cout << "Page has inherited dictionary." << std::endl;
				// 	// list included files
				// 	GPList<DjVuFile> included_files = djvu_file->get_included_files();
				// 	if (included_files.size() > 0) {
				// 		// find an included file corresponding to the inherited dictionary
				// 		for (GPosition i = included_files ; i; ++i) {
				// 			GP<DjVuFile> included_file = included_files[i];
				// 			if (included_file->fgjd) { // this is an inherited dictionary
				// 				std::cout << " It's an inherited dictionary containing " << dict_name << " containing "  << included_file->fgjd->get_shape_count() << " shapes." << endl;
				// 				dict_name = (string) included_file->get_url().fname();
				// 				break;
				// 			}
				// 		}
				// 	}
				// }
				//compute page dictionary parameters
				// std::map<int, int> page_shape_translation;

				string page_name = (string) djvu_file->get_url().fname();
				int sh_count = jimg->get_shape_count();
				int blit_count = jimg->get_blit_count();
				// store page dictionary

				// if (test_run)
				// 	std::cout << "Processing page shapes. " << std::endl;
				// std::cout << "normal shapes" << endl;
				// // store shapes
				// for (int i=inh_sh_count; i < sh_count;i++) {
				// 	JB2Shape shape = jimg->get_shape(i);
				// 	std::cout << page_number << " " << shape.bits->columns() << " "  << shape.bits->rows() << " "  << shape.bits->rowsize() << endl;
				// }

				if (test_run)
					std::cout << "Processing blits. " << std::endl;
				// store blits
				for (int i = 0; i < blit_count; i++) {
					JB2Blit *blit = jimg->get_blit(i);
					if (blit) {
						JB2Shape shape = jimg->get_shape(blit->shapeno);
						std::cout << ((blit->shapeno < inh_sh_count)?"s":"d") << "," << page_number << "," << i << "," << blit->shapeno  << "," << shape.bits->columns() << ","  << shape.bits->rows() << ","  << shape.bits->rowsize() << "," << blit->bottom << ","  << blit->left << endl;
					}
				}
			}
		}
	}
	return EXIT_SUCCESS;
}




void usage(char **argv) {
	std::cout << "Usage: " << argv[0] << "[-t <page #>] [-f <page #>] <filename>" << std::endl;
	std::cout << "Option -f: its argument specifies a page number from which the processing should start." << std::endl;
	std::cout << "Option -t: its argument specifies a limit to the number of pages processed by the program." << std::endl;
}

int main(int argc, char **argv) {
	try {
		char *filename, *doc_address;
		int c;
		bool  given_address = false;
		int page_from = 1 , page_to = -1;
		while ((c = getopt (argc, argv, "Tolicd:u:h:p:f:t:a:")) != -1)
			switch (c)
		    {
				case 'T':
					test_run = true;
					break;
				case 'f':
					page_from = atoi(optarg);
					break;
				case 't':
					page_to = atoi(optarg);
					break;
				case 'a':
					doc_address = optarg;
					given_address = true;
					break;
				case '?':
		    	 usage(argv);
		         return 1;
		       default:
		         usage(argv);
		         return 1;
		    }

		if (optind == argc -1) {
			filename = argv[optind];
		}

		if (!given_address) {
			doc_address = filename;
		}
		
		const GURL::Filename::UTF8 url(filename);


		GP<DjVuDocument> doc = DjVuDocument::create_wait(url);

		if (!doc) {
			std::cerr << "cannot open file" << std::endl;
			return -1;
		}

		return process_document(page_from, page_to, doc);
	}
	catch (exception& ex) {
		std::cout << "An exception occurred: " << ex.what() << "\n";
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

