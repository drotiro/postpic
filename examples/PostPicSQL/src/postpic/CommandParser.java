package postpic;

import java.awt.Image;
import java.awt.Toolkit;
import java.io.Console;
import java.io.FileOutputStream;
import java.io.IOException;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.util.Vector;
import javax.swing.JFrame;
import postpic.ui.ImageViewer;

public class CommandParser {
	
	enum QueryMode { SINGLE, THUMBS, PSQL, SAVE }
	enum ImageType { IMAGE, TEMPORARY_IMAGE }
		
	private Console c;
	private Connection conn;
	QueryMode mode = QueryMode.SINGLE;
	private ImageViewer v;
	private String saveFile;

	public CommandParser() {
		c = System.console();
	}

	public void connect(String args[]) throws SQLException {
		conn = DriverManager.getConnection(args[0], args[1], args[2]);
		conn.setAutoCommit(false);
	}
	
	/**
	 * PostPicSQL's entry point. It instantiates a CommandParser
	 * and runs its main loop. 
	 * @param args
	 * @throws SQLException 
	 */
	public static void main(String[] args) throws SQLException {
		CommandParser parser = new CommandParser();
		parser.connect(args);
		int res = parser.mainLoop();
		System.exit(res);		
	}

	public int mainLoop() {
		c = System.console();
		String line, cmd;
		String words[];
		printBanner();
		prompt();
		while((line = c.readLine()) != null) {
			words = line.split("\\s");
			cmd = words[0];
			if(isHelp(cmd)) { printHelp();  }
			else if("set".equalsIgnoreCase(cmd)) { parseCommand(words);  }
			else if(isQuit(cmd)) { return 0;  }
			else { parseQuery(line); System.gc(); }
			prompt();
		}
		return 0;
	}

	private boolean isQuit(String cmd) {
		return "quit".equalsIgnoreCase(cmd) || "\\q".equalsIgnoreCase(cmd);
	}

	private boolean isHelp(String cmd) {
		return "help".equalsIgnoreCase(cmd) || "\\h".equalsIgnoreCase(cmd);
	}

	private void prompt() {
		System.out.print("PostPicSQL> ");
	}

	private void parseQuery(String line)  {
		PreparedStatement ps = null;
		ResultSet rs = null;
		ResultSetMetaData rm;
		int col;
		String fname;
		Image im;
		Vector<Image> imgs = new Vector<Image>();
		
		try {
			ps = conn.prepareStatement(line);
			
			if(mode == QueryMode.PSQL) {
				ps.execute();
			} else if(mode != QueryMode.PSQL) {
				rs = ps.executeQuery();
				rm = rs.getMetaData();
				col = findImageColumn(rm);
				fname = rm.getColumnName(col);
				c.printf("Using column %s\n", fname);
				while(rs.next()) {
					if (mode==QueryMode.SAVE) {
						doSave((PGImage)rs.getObject(col));
						break;
					}
					im = getImage(rs, col);
					if(mode==QueryMode.SINGLE) {
						display(new Image[] {im});
						break;
					} 
					else {
						imgs.add(im);
					}
				}
			}
			if(mode==QueryMode.THUMBS) display( imgs.toArray(new Image[]{}));
		} catch (Exception e) {
			c.printf("ERROR: %s\n", e.getMessage());
		} finally {
			try { if(rs!=null) rs.close(); ps.close(); } catch (Exception ignored) { }			
		}
	}


	private void doSave(PGImage im) throws Exception {
		//not so efficient!
		byte [] imgdata = im.getRawData();
		c.printf("Saving %d bytes of data to file %s\n", imgdata.length, saveFile);
		FileOutputStream fos = new FileOutputStream(saveFile);
		fos.write(imgdata);
		fos.close();
		mode = QueryMode.SINGLE;
		showMode();
	}

	private Image getImage(ResultSet rs, int col) throws SQLException, IOException {
		PGImage pi = (PGImage) rs.getObject(col);
		return pi.getImage();
	}

	private void display(Image[] images) {
		//System.out.println("<<< Images: "+images.length+" >>>");
		ImageViewer v = getViewer();
		v.setImages(images);
		v.setVisible(true);
	}

	private synchronized ImageViewer getViewer() {
		if(v!=null) return v;
		v = new ImageViewer();
		v.setTitle("PostPicSQL results");
		v.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
		v.setSize(Toolkit.getDefaultToolkit().getScreenSize()); //screen size?
		return v;
	}

	private int findImageColumn(ResultSetMetaData m) throws SQLException {
		String ctn;
		int cc = m.getColumnCount();
		for(int i = 1; i <= cc; ++i) {
			ctn = m.getColumnTypeName(i);
			if(ctn.equals("image")) return i;
		}
		return -1;
	}

	private void parseCommand(String words[]) {
		if((words.length < 3) || !"mode".equalsIgnoreCase(words[1])) {
			printHelp();
			return;
		}
		String marg = words[2];
		if("single".equalsIgnoreCase(marg)) { mode = QueryMode.SINGLE; showMode(); }
		else if("thumbs".equalsIgnoreCase(marg)) { mode = QueryMode.THUMBS; showMode(); }
		else if("sql".equalsIgnoreCase(marg)) { mode = QueryMode.PSQL; showMode(); }
		else if("save".equalsIgnoreCase(marg)) {
			if (words.length > 3) {
				mode = QueryMode.SAVE;
				saveFile = words[3];
				showMode();
			} else c.printf("set mode save needs a <filename> argument\n");
		}
		else {
			c.printf("Invalid mode '%s', type 'help' for valid values\n", words[2]);
		}
	}

	private void showMode() {
		String msg = "Mode set to ";
		switch (mode) {
			case SINGLE: msg+="single"; break;
			case THUMBS: msg+="thumbs"; break;
			case SAVE: 	msg +="save. Will save to "+saveFile; break;
			default: msg+="sql"; break;
		}
		System.out.println(msg);
	}

	private void printHelp() {
		System.out.println("Available options:\n\thelp (\\h):\tprints this help message\n\tset mode <mode>:\tsets query mode\n");
		System.out.println("\t<query>:\texecutes the specified query\n\tquit (\\q):\texit this program");
		System.out.println("\nQuery modes:\n\tsingle:\tdisplays the image in the first record");
		System.out.println("\tthumbs:\tdisplays as many images fit in the output window");
		System.out.println("\tsave <filename>:\tsaves the image to the specified file");
		System.out.println("\tsql:\texecutes the query and displays server response");
	}

	private void printBanner() {
		System.out.println("PostPicSQL: a tool to query PostPic databases and view resulting images.");
		System.out.println("type 'help' to get basic usage information.");
	}

}
