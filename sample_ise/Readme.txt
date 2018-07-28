sample_ise测试流程：
	该sample测试mpi_ise组件双目广角拼接功能。创建ise组件，将两路图像传输给mpi_ise，ISE组件对图像进行拼接，
通过调用mpi获取ISE组件处理后的数据，到达指定次数后，停止运行并销毁ISE组件。也可以手动按ctrl-c，终止测试。
           如果需要获取ISE组件处理后的YUV数据，需要将sample_ise.c中的Save_Picture宏打开。

读取测试参数的流程：
	sample提供了sample_ise.conf，测试参数都写在该文件中。
	启动sample_ise时，在命令行参数中给出sample_ise.conf的具体路径，sample_ise会读取sample_ise.conf，完成参数解析。
	然后按照参数运行测试。
从命令行启动sample_ise的指令：
	./sample_ise -path ./sample_ise.conf
	"-path ./sample_ise.conf"指定了测试参数配置文件的路径。

测试参数的说明：
1.auto_test_count：指定自动化测试次数
2.process_count: 指定ISE组件处理次数
3.pic_width：指定源图像宽度
4.pic_height：指定源图像高度
5.pic_stride：指定源图像的stride值，该值必须是32的倍数
6.pic_frame_rate：指定发送源图像的帧率
7.pic0_file_path：指定第一个源图像的路径
8.pic1_file_path：指定第二个源图像的路径
9.ise_port_num：指定ISE组件端口个数
10.ise_output_file_path：指定ISE组件输出图像的路径
11.ise_wfov: 指定镜头水平方向视场角
12.ise_hfov: 指定镜头垂直方向视场角
13.ise_ov: 指定重叠区宽度，该值必须满足以下条件：ov < 1/10 * ise_chn0_width且小于等于320，同时必须是16的倍数 
14.ise_portx_width：指定拼接处理后图像的宽度
15.ise_portx_height：指定拼接处理后图像的高度
16.ise_portx_stride：指定拼接处理后图像的stride值，该值必须是32的倍数
17.ise_portx_flip_enable：指定是否使能图像翻转
18.ise_portx_mirror_enable：指定是否使能图像镜像
注：每个ISE端口图像翻转和镜像只能使能其中一个,测试时需要拷贝yuv文件到测试路径下面，程序默认打开程序所在文件夹的yuv文件

