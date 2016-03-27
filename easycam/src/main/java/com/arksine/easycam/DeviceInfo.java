package com.arksine.easycam;

/**
 * Created by Eric on 1/6/2016.
 */
public class DeviceInfo {

	public enum DeviceStandard {
		NTSC(0),
		PAL(1);

		private int index;
		DeviceStandard(int idx) {
			this.index = idx;
		}

		public int getIndex() {
			return index;
		}
	};

	/**
	 * TODO:  May want to support more pixel formats, particularly variations of ARGB.  The issue
	 * is understanding how the device is writing bytes to memory, and how Android expects to read
	 * them from memory.  The currently supported formats are the most popular for analog capture
	 * devices.
	 */
	public enum PixelFormat {
		YUYV(0),
		UYVY(1),
		NV21(2),
		YV12(3),
		RGB565(4);

		private int index;
		PixelFormat(int idx) {
			this.index = idx;
		}

		public int getIndex() {
			return index;
		}
	};

	/**
	 * The enumeration below represents how a device driver writes a frame to memory.  See the V4L2
	 * enum v4l2_field documentation for more detailed explanation.
	 */
	public enum FieldType {
		NONE(0),                // Progressive Scan
		TOP_ONLY(1),            // Only Top (odd) fields are written to memory
		BOTTOM_ONLY(2),         // Only Bottom (even) fields are written to memory
		INTERLACED(3),          // Memory contains both frames, withe fields interleaved
		SEQUENTIAL_TB(4),       // Memory contains both frames, with top fields stored first in memory
		SEQUENTIAL_BT(5),       // Memory cotains both frames, with bottom fields stored first in memory
		ALTERNATE(6);           // Fields are separated and written to memory in different buffers, in temporal order

		private int index;
		FieldType(int idx) {
			this.index = idx;
		}

		public int getIndex() {
			return index;
		}
	};

	public enum DeintMethod {
		NONE(0),
		DISCARD(1),
		BOB(2),
		BLEND(3);

		private int index;
		DeintMethod(int idx) {
			this.index = idx;
		}

		public int getIndex() {
			return index;
		}
	};

	private String description = "default";
	private String driver = "default";
	private String vendorID = "0000";
	private String productID = "0000";
	private String location = "/dev/video0";
	private int frameWidth = 720;
	private int frameHeight = 480;
	private int numBuffers = 2;
	private DeviceStandard devStd = DeviceStandard.NTSC;
	private PixelFormat pixFmt = PixelFormat.YUYV;
	private FieldType field = FieldType.INTERLACED;
	private DeintMethod deinterlace = DeintMethod.NONE;
	private int input = -1;

	// Empty Constructor for Objects needing initial values
	DeviceInfo() {}

	DeviceInfo(String desc, String drv, String vID, String pID, String loc, int fWidth, int fHeight,
			   int bufs, int ipt, DeviceStandard std, PixelFormat fmt, FieldType fld,
			   DeintMethod deint){
		description = desc;
		driver = drv;
		vendorID = vID;
		productID = pID;
		location = loc;
		frameWidth = fWidth;
		frameHeight = fHeight;
		numBuffers = bufs;
		input = ipt;
		devStd = std;
		pixFmt = fmt;
		field = fld;
		deinterlace = deint;
	}

	public String getDescription() {
		return description;
	}

	public void setDescription(String description) {
		this.description = description;
	}

	public String getDriver() {
		return driver;
	}

	public void setDriver(String driver) {
		this.driver = driver;
	}

	public String getLocation() {
		return location;
	}

	public void setLocation(String location) {
		this.location = location;
	}

	public String getProductID() {
		return productID;
	}

	public void setProductID(String productID) {
		this.productID = productID;
	}

	public String getVendorID() {
		return vendorID;
	}

	public void setVendorID(String vendorID) {
		this.vendorID = vendorID;
	}

	public int getFrameWidth() {
		return frameWidth;
	}

	public void setFrameWidth(int frameWidth) {
		this.frameWidth = frameWidth;
	}

	public int getFrameHeight() {
		return frameHeight;
	}

	public void setFrameHeight(int frameHeight) {
		this.frameHeight = frameHeight;
	}

	public int getNumBuffers() {
		return numBuffers;
	}

	public void setNumBuffers(int numBuffers) {
		this.numBuffers = numBuffers;
	}

	public int getInput() {
		return input;
	}

	public void setInput(int input) {
		this.input = input;
	}

	public DeviceStandard getDevStd() {
		return devStd;
	}

	public int getDevStdIdx() {
		return devStd.getIndex();
	}

	public void setDevStd(DeviceStandard std) {
		this.devStd = std;
	}

	public PixelFormat getPixFmt() {
		return pixFmt;
	}

	public int getPixFmtIdx() {
		return pixFmt.getIndex();
	}

	public void setPixFmt(PixelFormat pixFmt) {
		this.pixFmt = pixFmt;
	}

	public DeintMethod getDeinterlace() {
		return deinterlace;
	}

	public int getDeinterlaceIdx() {
		return deinterlace.getIndex();
	}

	public void setDeinterlace(DeintMethod deinterlace) {
		this.deinterlace = deinterlace;
	}

	public FieldType getFieldType() {
		return field;
	}

	public int getFieldTypeIdx() {
		return field.getIndex();
	}

	public void setFieldType(FieldType field) {
		this.field = field;
	}

}