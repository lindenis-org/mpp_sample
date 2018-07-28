BDII：Binocular depth information image

编译此例程需要修改 middleware/config/mpp_config.mk 文件：MPPCFG_BDII := Y

sample_cve_BDII测试流程：
    sample_cve_BDII:从yuv原始数据文件xxx.yuv中读取视频帧，送到BDII库中进行分析处理得到相关结果，并输出景深信息图。

读取测试参数的流程：
    sample提供了两种方法指定测试参数，一种是直接用命令行参数输入，一种是从配置文件读取，
    两种方式可以同时存在，不过命令行输入的优先级要高于配置文件（也就是说同样的项目，命令行配置会覆盖配置文件里面的配置）,
    命令行可以不输入全部的参数，只输入需要修改的参数，比如只输入宽高参数，其余的参数与配置文件保持一致。
    1. sample_cve_BDII.conf，测试参数都写在该文件中：
        启动sample_cve_BDII时，在命令行参数中给出sample_cve_BDII.conf的具体路径，sample_cve_BDII会读取sample_cve_BDII.conf，完成参数解析。
    2. 命令行输入：
        "exec [-h|--help] [-p|--path]\n"
        "   <-h|--help>: 打印帮助信息\n"
        "   <-p|--path>   <args>: 指定配置文件的路径\n"
        "   <-x|--width>  <args>: 设置图像宽度\n"
        "   <-y|--height> <args>: 设置图像的高度\n"
        "   <-e|--format> <args>: 设置图像格式\n"
        "   <-l|--left>   <args>: 左边图像数据源\n"
        "   <-r|--right>  <args>: 右边图像数据源\n"
        "   <-o|--output> <args>: 输出结果到此文件\n"

    从命令行启动sample_cve_BDII的指令：
    ./sample_cve_BDII --path /mnt/extsd/sample_cve_BDII.conf || ./sample_cve_BDII -x 384 -y 288 -e nv21 -l left.yuv -r right.yuv

测试参数的说明：
(1)src_file：指定原始yuv文件的路径
(2)pic_width ：指定原始视频文件的图像帧宽度
(2)pic_height：原始视频文件的图像帧高度
(4)pic_format: 原始视频文件的像素格式
(5)src_file_l: 左边图像
(6)src_file_r: 右边图像
(7)output_file: 输出结果
