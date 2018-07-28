sample_EVE测试流程：
	从yuv原始数据文件xxx.yuv中读取视频帧，送到eve库中进行分析处理得到相关结果，并输出人脸识别的结果（数量、位置等）。

编译此例程需要修改 middleware/config/mpp_config.mk 文件：MPPCFG_EVEFACE := Y

读取测试参数的流程：
	sample提供了两种方法指定测试参数，一种是直接用命令行参数输入，一种是从配置文件读取，
	两种方式可以同时存在，不过命令行输入的优先级要高于配置文件（也就是说同样的项目，命令行配置会覆盖配置文件里面的配置）,
	命令行可以不输入全部的参数，只输入需要修改的参数，比如只输入宽高参数，其余的参数与配置文件保持一致。
	1. sample_EVE.conf，测试参数都写在该文件中：
		启动sample_EVE时，在命令行参数中给出sample_EVE.conf的具体路径，sample_EVE会读取sample_EVE.conf，完成参数解析。
	2. 命令行输入：
        "exec [-h|--help] [-p|--path]\n"
        "   <-h|--help>: 打印帮助信息\n"
        "   <-p|--path>   <args>: 指定配置文件的路径\n"
        "   <-x|--width>  <args>: 设置图像宽度\n"
        "   <-y|--height> <args>: 设置图像的高度\n"
		"   <-f|--framerate>  <args>: set the video frame\n"
		"   <-t|--testcnt><args>: set the test frame count number\n"
		"   <-m|--pic>    <args>: point to the yuv file\n"
		"   <-e|--format> <args>: set the video format\n"
		"   <-d|--device> <args>: set the device number\n"
		"   <-o|--output> <args>: output result to this file\n"

从命令行启动sample_EVE的指令：
	./sample_EVE --path /mnt/extsd/sample_EVE.conf || ./sample_EVE -x 640 -y 360 -f 25 -e nv21 -m face.yuv

按下ctrl+c键退出测试。

注意：测试的时候需要将classifier文件夹拷贝至程序运行的文件夹内，程序会自动去该文件夹下读取EVE模型配置。

测试参数的说明：
(1)src_file：指定原始yuv文件的路径
(2)pic_width ：指定原始视频文件的图像帧宽度
(2)pic_height：原始视频文件的图像帧高度
(4)pic_format: 原始视频文件的像素格式
(5)frame_rate: 帧率
(6)output_file：结果存储文件路径
(7)test_frame：测试总帧数
(8)dev：vipp设备号
