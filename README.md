Allwinner V5 mpp sample code. Feel free to download to your board and build it.

## Downloading

On your board, run the following commands to download the latest vesion of mpp sample.

```
$ git clone https://github.com/lindenis-org/mpp_sample.git
```

## Building

Build each sample on your board as your need.

```
$ cd mpp_sample/sample_virvi2vo
$ make
```

## Running

The sample requires root privileges to run. The arguments of each sample can be specified by `-path`. Note that modify the default value as your board setup.

```
$ sudo su
# ./sample_virvi2vo -path sample_virvi2vo.conf
```

## Troubleshooting

Feel free to ask for help in the [forum](http://forum.lindeni.org/).
