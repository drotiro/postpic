/*********************************************************************
 PostPic - An image-enabling extension for PostgreSql
 (C) Copyright 2010 Domenico Rotiroti

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as
 published by the Free Software Foundation, version 3 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 A copy of the GNU Lesser General Public License is included in the
 source distribution of this software.

*********************************************************************/
package postpic;

import java.awt.Image;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.sql.SQLException;
import javax.imageio.ImageIO;

import org.postgresql.util.PGbytea;
import org.postgresql.util.PGobject;

public class PGImage extends PGobject {
	
	private static final long serialVersionUID = 4395514282029013752L;

	private byte[] bytea;	

	public PGImage() {
		setType("image");
	}
	
	@Override
	public void setValue(String value) throws SQLException {
		bytea = PGbytea.toBytes(value.getBytes());
	}
			
	public Image getImage() throws SQLException, IOException {
		Image i = ImageIO.read(new ByteArrayInputStream(bytea));
		return i;
	}
	
	public byte[] getRawData() {
		return bytea;
	}
}
