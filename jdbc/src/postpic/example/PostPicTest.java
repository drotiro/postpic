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
package postpic.example;

import java.sql.*;
import java.awt.Image;
import javax.swing.ImageIcon;
import javax.swing.JFrame;
import javax.swing.JLabel;

import postpic.PGImage;
import postpic.PostPic;


public class PostPicTest {
	
	String host = "localhost", 
		user = "postpic", 
		password = "postpic", 
		port = "5432",
		database = "postpic";
	
	public String getHost() {
		return host;
	}

	public void setHost(String host) {
		this.host = host;
	}

	public String getUser() {
		return user;
	}

	public void setUser(String user) {
		this.user = user;
	}

	public void setPassword(String password) {
		this.password = password;
	}

	public void setPort(String port) {
		this.port = port;
	}

	public void setDatabase(String database) {
		this.database = database;
	}
	
	public Connection getConnection() throws SQLException {
		Connection res = DriverManager.getConnection("jdbc:postgresql://"+host+":"+port+"/"+database, user, password);
		res.setAutoCommit(false);
		return res;
	}

	// TEST METHODS
	
	public static void main(String[] args) throws Exception {
		PostPicTest c = new PostPicTest();
		Connection conn = c.getConnection();
		PostPic pp = new PostPic(conn);
		System.out.println("Version: "+pp.getVersionString()+" ("+pp.getPostPicRelease()
				+"."+pp.getPostPicMajor()+"."+pp.getPostPicMinor()+")");
		if(pp.isRegistered()) {
			System.out.println("Autoreg OK");
		} else {
			System.out.println("Autoreg failed, trying manual");
			pp.registerManual();
		}
		String sql = "select the_img as image, name , date(the_img) as ts from images i /*join album_images a on a.iid=i.iid and aid=2 where size(the_img)>1600*/";
		PreparedStatement st = conn.prepareStatement(sql);
		ResultSet rs = st.executeQuery();
		while(rs.next()) {
			Object o = rs.getObject("image");
			if(o instanceof PGImage) {
				PGImage i = (PGImage)o;
				Image img = i.getImage();
				display(img, rs.getString("name"));
			} else {
				System.out.println("Registration failed.\nImage class: "+o.getClass().getCanonicalName());
			}
		}
		rs.close();
		st.close();
		conn.close();
	}
	
	static void display(Image i, String name) {
		JFrame f = new JFrame(name);
		f.getContentPane().add(new JLabel(new ImageIcon(i)));
		f.setSize(i.getWidth(null), i.getHeight(null));
		f.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
		f.setVisible(true);
	}
}
