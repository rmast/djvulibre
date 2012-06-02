/***
 * 
 * Copyright © 2011 -- Grzegorz Chimosz.
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

