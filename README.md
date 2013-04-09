# pp
## Examples
### Compression progress bar

```
% pp test-file | gzip > test-file.gz
 752.4 Mib / 3.5 Gib  [=============---------------- 20% -----------------------------] 20.9 Mib/s  (eta 02m 16s)
```

### Linear HDD read speed
```
% pp -r /dev/ad8
   4.9 Gib / 40 Gib   [=======---------------------- 12% -----------------------------] 73.3 Mib/s  (eta 08m 10s)
```

### Copying
```
% pp test-file > test-file.copy
   2.8 Gib / 3.5 Gib  [============================= 79% ================-------------] 52.8 Mib/s  (eta 00m 14s)
```

### Line-mode: grep progress
```
% pp -l test-file | grep needle > needles
  12.6 Mln / ~52.4 Mln [==============--------------- 23% ----------------------------] 783.7 Kln/s (eta 00m 50s)
```

## Usage
```
pp [-lsr] [file-to-read]
```

 * `-l` line mode, show progress based on approximate size of file in lines
 * `-r` read-only mode: do not write data to STDOUT
 * `-s` manually set size of pipe (`10Mb`, `1Gb`, `1Tb` etc)

## Installation
```
% git clone git://github.com/avz/pp.git
% cd pp
% make
% sudo make install
```
