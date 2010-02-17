package postpic;

import java.awt.Image;
import java.io.IOException;
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
	private int width, height;
	private Timestamp date;

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
			try { date = Timestamp.valueOf(parts[1]); } catch (Exception e) {date = new Timestamp(0); }
			width = Integer.parseInt(parts[2]);
			height = Integer.parseInt(parts[3]);
		}
	}
	
	/*
	 * It's sad that an object returned from a ResultSet
	 * can't inherit the connection...
	 * Ask the user to provide it...
	 */
	public Image getImage(PGConnection conn) throws SQLException, IOException {
		LargeObjectManager lm = conn.getLargeObjectAPI();
		LargeObject lo = lm.open(loid);
		Image i = ImageIO.read(lo.getInputStream());
		lo.close();
		return i;
	}
}
