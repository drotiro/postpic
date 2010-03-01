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
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Timestamp;

import javax.imageio.ImageIO;

import org.postgresql.PGConnection;
import org.postgresql.largeobject.LargeObject;
import org.postgresql.largeobject.LargeObjectManager;
import org.postgresql.util.PGobject;

public class PGImage extends PGobject {
	
	private static final long serialVersionUID = 4395514282029013752L;
	
	private long loid;
	private int width, height, iso;
	double f_number, exposure_t;
	private Timestamp date;
	private PGConnection conn;
	
	public int getIso() {
		return iso;
	}

	public double getFNumber() {
		return f_number;
	}

	public double getExposureTime() {
		return exposure_t;
	}

	/*
	 * It's sad that an object returned from a ResultSet
	 * can't inherit the connection...
	 * Ask the user to provide it...
	 */
	public void setConn(Connection conn) {
		this.conn = (PGConnection)conn;
	}

	public int getWidth() {
		return width;
	}

	public int getHeight() {
		return height;
	}

	public Timestamp getDate() {
		return date;
	}

	public PGImage() {
		setType("image");
	}
	
	public String getValue() {
		return value;
	}
	
	public void setValue(String value) {
		String [] parts = value.split("\\|");
		loid = Long.parseLong(parts[0]);
		if(parts.length >= 4) {
			date = null;
			width = height = iso = 0;
			exposure_t = f_number = -1;			
			try { 
				date = parseTimestamp(parts[1]);
				width = Integer.parseInt(parts[2]);
				height = Integer.parseInt(parts[3]);
				f_number = Double.parseDouble(parts[4]);
				exposure_t = Double.parseDouble(parts[5]);
				iso = Integer.parseInt(parts[6]);
			} catch (Exception ign) {
			}
		}
	}
		
	private Timestamp parseTimestamp(String ts) {
		try {
			return Timestamp.valueOf(ts);
		} catch (Exception parseEx) {
			return null;
		}
	}

	public Image getImage() throws SQLException, IOException {
		LargeObjectManager lm = conn.getLargeObjectAPI();
		LargeObject lo = lm.open(loid);
		Image i = ImageIO.read(lo.getInputStream());
		lo.close();
		return i;
	}
	
	public Image getThumbnail(int size) throws SQLException, IOException {
		Connection c = (Connection) conn;
		c.prepareStatement("begin").execute();
		PreparedStatement ps = c.prepareStatement(
				"select image_thumbnail(image_create_from_loid("+loid+"), "+size+") as thumb");
		ResultSet rs = ps.executeQuery();
		rs.next();
		byte[] imgb = (byte[]) rs.getObject("thumb");
		c.prepareStatement("end").execute();
		return ImageIO.read(new ByteArrayInputStream(imgb));
	}
	
	public Image getSquare(int size) throws SQLException, IOException {
		Connection c = (Connection) conn;
		c.prepareStatement("begin").execute();
		PreparedStatement ps = c.prepareStatement(
				"select image_square(image_create_from_loid("+loid+"), "+size+") as thumb");
		ResultSet rs = ps.executeQuery();
		rs.next();
		byte[] imgb = (byte[]) rs.getObject("thumb");
		c.prepareStatement("end").execute();
		return ImageIO.read(new ByteArrayInputStream(imgb));
	}
}
