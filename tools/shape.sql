-- Copyright © 2012 Piotr Sikora <piotr.sikora@student.uw.edu.pl>
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful, but
-- WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
-- General Public License for more details.

-- You should have received a copy of the GNU General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.

-- varchar, datetime, integer, char

create table shapes (
	id INT not null auto_increment primary key,
	page_id INT not null,
	parent_id INT not null,
	bits LONGBLOB,
	bbox_top INT,
	bbox_left INT,
	bbox_right INT,
	bbox_bottom INT
);

create table blits (
	id INT not null auto_increment primary key,
	page_id INT not null,
	shape_id INT not null,
	b_left SMALLINT UNSIGNED not null,
	b_bottom SMALLINT UNSIGNED not null
);


create table documents (
	id INT not null auto_increment primary key,
	document varchar(60) not null
);

create table pages (
	id INT not null auto_increment primary key,
	page_number INT not null,
	document_id INT not null
);



