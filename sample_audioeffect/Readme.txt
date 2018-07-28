sample_audioeffect测试流程：
	根据配置参数采集对应的pcm数据，经音效库处理后，将其送往ao输出音频。用户须通过命令行手动控制开启/停止某种音效库功能，拿耳机视听声音效果变化。

读取测试参数的流程：
	sample提供了sample_audioeffect.conf，测试参数包括：采样率(pcm_sample_rate)、通道数目(pcm_channel_cnt)、数据位宽(pcm_bit_width)。
	启动sample_audioeffect时，在命令行参数中给出sample_audioeffect.conf的具体路径，sample_audioeffect会读取该文件，完成参数解析。
	然后按照参数运行测试,用户须根据命令行提示来测试某种具体的音效。

从命令行启动sample_ai的指令：
	./sample_audioeffect -path /mnt/extsd/sample_audioeffect/sample_audioeffect.conf
	"-path /mnt/extsd/sample_audioeffect/sample_audioeffect.conf"指定了测试参数配置文件的路径。

测试参数的说明：
(1)pcm_sample_rate：指定采样率，通常设置为8000。
(2)pcm_channel_cnt：指定通道数目，通常为1或2。
(3)pcm_bit_width：指定位宽，必须设置为16。