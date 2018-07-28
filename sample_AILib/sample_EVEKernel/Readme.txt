sample_eve测试流程：
	从yuv(sample中格式限定为yuv420p)原始数据文件xxx.yuv中读取视频帧，送到eve库中进行分析处理得到相关结果，并输出结果文件。
	sample支持两种模式取源yuv数据，一种是自己用fread读取，另一种是调用库yuv处理接口。

编译此例程需要修改 middleware/config/mpp_config.mk 文件：MPPCFG_EVEFACE := Y

读取测试参数的流程：
	sample提供了sample_eve.conf，测试参数都写在该文件中。
	启动sample_eve时，在命令行参数中给出sample_eve.conf的具体路径，sample_eve会读取sample_eve.conf，完成参数解析。
	然后按照参数运行测试。
	从命令行启动sample_eve的指令：
	./sample_evekernel -path /mnt/extsd/sample_eve.conf
	"-path /mnt/extsd/sample_eve.conf"指定了测试参数配置文件的路径。

测试参数的说明：
(1)src_data_file：指定原始yuv文件的路径
(2)src_w：指定原始视频文件的图像帧宽度
(2)src_h：原始视频文件的图像帧高度
(4)src_pixfmt: 原始视频文件的像素排列格式
(5)test_frame_rate: 帧率(模拟)
(6)test_frame_num: 从源文件取的待分析帧数
(7)result_out_file: 识别结果输出文件路径