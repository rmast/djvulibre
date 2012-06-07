/***
 * 
 * Copyright © 2011 -- Grzegorz Chimosz, 2012 -- Piotr Sikora.
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

#include <DjVuDocument.h>
#include <DjVuImage.h>
#include <JB2Image.h>

using namespace DJVU;

int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cout << "usage: inhdict <filename>" << std::endl;
		return -1;
	}
	int total_shapes = 0, inh_dict_shapes = 0, last_dict = -1;

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
		for(int page = 0; page < pages; page++) {
			std::cout << "Page " << page << " shape count: ";

			GP<DjVuImage> dimg = doc->get_page(page);
			if (!dimg) {
				std::cout << "get_page failed";
			} else {
				GP<JB2Image> jimg = dimg->get_fgjb();
				if (!jimg) {
					std::cout << "get_fgjb failed";
				} else {
					std::cout << "total = " << jimg->get_shape_count() << " ; inherited = ";
					total_shapes += jimg->get_shape_count();
					if (jimg->get_inherited_dict()) {
						std::cout <<
							jimg->get_inherited_shape_count()
							<< " ; page only = "
							<< jimg->get_shape_count() - jimg->get_inherited_shape_count() ;
						if (jimg->get_inherited_shape_count() != last_dict) {
							last_dict = jimg->get_inherited_shape_count();
							inh_dict_shapes += last_dict;
							
						} else {
							total_shapes -= last_dict;
						}
						found++;
					} else {
						std::cout << "inherited dictionary not found ; page only = " 
							  << jimg->get_shape_count();
					}
				}
			}

			std::cout << std::endl;
		}
		std::cout <<
			std::endl <<
			"Summary: document contains" << std::endl <<
			total_shapes << " total shapes" << std::endl <<
			inh_dict_shapes << " shapes in inherited dictionaries" << std::endl <<
			total_shapes - inh_dict_shapes << " shapes in page-only dictionaries "<< 
			
			std::endl;
		
	} G_CATCH(ex) {
		ex.perror();
		return 1;
	}
	G_ENDCATCH;

	return 0;
}

