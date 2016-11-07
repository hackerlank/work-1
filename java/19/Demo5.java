import java.io.File;
import java.sql.Date;
import java.text.SimpleDateFormat;

/*
	getName()	获取文件或者文件夹的名称，不包含上级路径
	getPath()	返回绝对路径，可以是相对路径，但是目录要指定
	getAbsolutePath()	获取文件的绝对路径，与文件是否存在没关系
	length()	获取文件的大小(字节数), 如果文件不存在返回OL，如果是文件夹也返回0L
	getParent()	返回此抽象路径名父目录的路径名字符串; 如果路径名没有指定父目录，则返回null
	lastModified()	获取最后一次被修改的时间
*/

public class Demo5 {
	public static void main(String[] args) {
		File file = new File("../../a.txt");
		System.out.println("文件名：" + file.getName());
		System.out.println("获取绝对路径：" + file.getPath());
		System.out.println("获取文件的大小(字节为单位)" + file.length());
		System.out.println("获取文件的父路径" + file.getParent());

		//使用毫秒值转换成Date对象
		long lastModified = file.lastModified();
		Date date = new Date(lastModified);

		SimpleDateFormat dateFormate = new SimpleDateFormat("yyyy年MM月dd日 HH:mm:ss");
		System.out.println("最后一次的修改时间:" + dateFormate.format(date));
	}
}
