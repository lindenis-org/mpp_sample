sample_virvi2opencv2vo测试流程：
	这是一个简单的通过opencv做运动检测的sample。运行本sample时，如果摄像头采集的画面静止不动，则全为黑屏，如果有运动的物体，则显示运动物体的轮廓。当实时图像帧差的均值差大于设置的灵敏度值时就出现以下打印，表明图像中有运动的物体或者图像发生明显变化。
	“Something is moving! mean_diff:xx"
	本sample打开sample_virvi2opencv2vo.conf指定的dev节点,mpi_vi采集图像,并且调用opencv进行处理，把处理后的结果通过mpi_vo组件显示在HDMI显示器上。
	
	到达测试次数后,停止运行并销毁相关组件,可以手动按ctrl-c，终止测试。

读取测试参数的流程：
    sample提供了sample_virvi2opencv2vo.conf，测试参数都写在该文件中。
启动sample_virvi2opencv2vo时，在命令行参数中给出sample_virvi2opencv2vo.conf的具体路径，sample_virvi2opencv2vo会读取sample_virvi2opencv2vo.conf，完成参数解析。
然后按照参数运行测试。
         从命令行启动sample_virvi2opencv2vo的指令：
        ./sample_virvi2opencv2vo -path ./sample_virvi2opencv2vo.conf
        "-path ./sample_virvi2opencv2vo.conf"指定了测试参数配置文件的路径。

测试参数的说明：
(1)auto_test_count:指定自动测试次数
(2)get_frame_count:指定每次测试获取图像的次数
(3)dev_num：指定VI Dev设备节点 
(4)pic_width：指定camera采集的图像宽度
(5)pic_height：指定camera采集的图像高度
(6)frame_rate:指定camera采集图像的帧率
(7)pic_format:指定camera采集图像像素格式
(8)enable_disp_color:是否显示图像的颜色
(9)moving_detect_sensitivity:运动检测灵敏度设置，大于等于1即可，1是最灵敏。