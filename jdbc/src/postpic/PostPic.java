package postpic;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import org.postgresql.PGConnection;
import org.postgresql.jdbc2.AbstractJdbc2Connection;


/*
 * The PostPic class provides basic version 
 * information and some help for users 
 * without a driver supporting autoregistration
 */
public class PostPic {

	private Connection conn;

	public PostPic(Connection conn) {
		this.conn = conn;
	}
	
	public String getVersionString() throws SQLException {
		return query("select postpic_version()");
	}

	public int getPostPicRelease() throws SQLException {
		return Integer.parseInt(query("select postpic_version_release()"));
	}

	public int getPostPicMajor() throws SQLException {
		return Integer.parseInt(query("select postpic_version_major()"));
	}

	public int getPostPicMinor() throws SQLException {
		return Integer.parseInt(query("select postpic_version_minor()"));
	}
	
	/*
	 * Manual registration, for older jdbc drivers
	 */
	public void registerManual() throws SQLException {
		PGConnection pc = (PGConnection) conn;
		pc.addDataType("image", PGImage.class);
	}

	public boolean isRegistered() throws SQLException {
		AbstractJdbc2Connection pc = (AbstractJdbc2Connection) conn;
		return PGImage.class.equals(pc.getTypeInfo().getPGobject("image"));
	}
	
	private String query(String sql) throws SQLException {
		PreparedStatement ps = conn.prepareStatement(sql);
		ResultSet rs = ps.executeQuery();
		rs.next();
		String res = rs.getString(1);
		rs.close();
		ps.close();
		return res;		
	}
}
