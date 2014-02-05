PostPic: A PostgreSQL extension for image-processing
====================================================

PostPic is an extension for the open source PostgreSQL dbms that enables
image processing inside the database, like PostGIS does for spatial data.
It adds the new 'image' type to the SQL, and several functions to process
images and to extract their attributes.

Eg.

      select * from images 
               where date(the_img) > '2009-01-01'::date
               and size(the_img) > 1600;

Traditional relational clauses can be combined in queries with image-related
ones, and basic image processing can be performed in SQL.


Resources
---------

PostPic is hosted at [GitHub](http://github.com/drotiro/postpic), 
there you can find all the code releases and the project 
[Wiki](http://wiki.github.com/drotiro/postpic/>) 
with detailed documentation about the project.


Contents of this package
------------------------

This package contains the following:

 * __server__: sources for the server extension
 * __utils__: utilities for client-side loading of images, etc.
 * __examples__: a sql script to create sample tables and functions


Copyright and license
---------------------

PostPic is Copyright (C) 2010 Domenico Rotiroti.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    A copy of the GNU Lesser General Public License is included in the
    source distribution of this software.
