package postpic.ui;

import java.awt.Graphics;
import java.awt.Image;
import java.awt.Insets;

import javax.swing.JFrame;

@SuppressWarnings("serial")
public class ImageViewer extends JFrame {
	final static int gap = 5;
	Image [] imgs;
	
	public void setImages(Image [] imgs) {
		this.imgs = imgs;
		repaint();
	}	

	@Override
	public void paint(Graphics g) {
		super.paint(g);
		
		Insets in = getInsets();
		int x = in.left + gap, y = in.top + gap, curMaxH = 0;
		for (int i = 0; i < imgs.length; ++i) {
			curMaxH = Math.max(curMaxH, imgs[i].getHeight(this));
			g.drawImage(imgs[i], x, y, this);
			//update x and y
			x += imgs[i].getWidth(this)+gap;
			if(x > (getWidth() -gap - in.right)) { // should take care of next img's width 
				x = in.left + gap;
				y += (curMaxH+gap);
				curMaxH = 0;
			}
		}
	}
}
