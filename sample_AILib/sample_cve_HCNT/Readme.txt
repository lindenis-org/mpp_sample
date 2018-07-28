HCNT: 人员计数

编译此例程需要修改 middleware/config/mpp_config.mk 文件：MPPCFG_HCNT := Y

sample_cve_HCNT测试流程：
	从YUV文件取视频帧，送到HCNT库中进行分析处理得到相关结果（包括目标个数，目标位置等等），输出每个目标的位置以及矩形框等。

读取测试参数的流程：
    sample提供了两种方法指定测试参数，一种是直接用命令行参数输入，一种是从配置文件读取，
    两种方式可以同时存在，不过命令行输入的优先级要高于配置文件（也就是说同样的项目，命令行配置会覆盖配置文件里面的配置）,
    命令行可以不输入全部的参数，只输入需要修改的参数，比如只输入宽高参数，其余的参数与配置文件保持一致。
    1. sample_cve_HCNT.conf，测试参数都写在该文件中：
        启动sample_cve_HCNT时，在命令行参数中给出sample_cve_HCNT.conf的具体路径，sample_cve_HCNT会读取sample_cve_HCNT.conf，完成参数解析。
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

从命令行启动sample_cve_HCNT的指令：
./sample_cve_HCNT -h
./sample_cve_HCNT --path /mnt/extsd/sample_cve_HCNT.conf || ./sample_cve_HCNT -x 640 -y 360 -e nv21 -t 4

测试时需要拷贝hcntParam文件夹到测试路径下面，程序默认打开程序所在文件夹下的hcntParam里面的xml文件

测试参数的说明：
(1)src_file：指定原始yuv文件的路径
(2)pic_width ：指定原始视频文件的图像帧宽度
(2)pic_height：原始视频文件的图像帧高度
(4)pic_format: 原始视频文件的像素格式
(5)frame_rate: 帧率
(6)output_file：结果存储文件路径
(7)test_frame：测试总帧数
(8)dev：vipp设备号
