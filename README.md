原版大气层迷你整合包

感谢论坛大佬的各种教程，小弟从中学习，发现制作大气层整合包就像搭积木，除了核心必须外，其它都可以任意添加删减。

（1）大气层增强包=大气层迷你包+特斯拉插件包+相册NRO软件包，按照顺序将特斯拉插件包与NRO软件包分别覆盖到迷你包中，并修改/config/fastcfwswitch/设定config.ini为config.ini就完成。

（2）迷你包是必须的，特斯拉插件包和NRO软件包均非必须，可以根据需要精简——原版大气层增强整合包可精简组件说明

【说明】

大气层迷你整合包取原版程序，自编配置和引导，支持Switch全机型，迷你包不包含特斯拉等插件，相册只有daybreak，可作底包自由添加插件（见可精简组件说明）。

默认启动90dns与隐藏序号，如需联机进ofw sysnand正版系统。如需cfw sysnand破解系统联机，删除atmosphere/hosts/sysmmc.txt和修改exosphere.ini中blank_prodinfo_sysmmc=0

hbmenu.nsp为万能前端，通过游戏安装器安装

【更新与启动方法】

（1）删除原TF卡上atmosphere，bootloader，config，sept和switch五个文件夹，之后再将压缩包中的文件全选复制到TF卡覆盖

（2）软破机，大气层的注入器内置Payload需Hekate4.2以上，SX的注入器直接可以用。

短接+注入开机，点击launch，根据选择进入

cfw sysnand：真实系统破解状态（fss0引导）

cfw eumnand：虚拟系统破解状态（fss0引导）

ofw sysnand：真实系统不破解状态（fss0引导）

点击more configs，根据选择进入

cfw auto：自动识别真实/虚拟进入破解（fusee引导）

sxos：引导sxos，需要换sxos3.1的boot.dat

lakka：lakka模拟器，需要自行添加

（3）硬破机直接点击电源键开机，之后与软破机操作一样。

<img src="https://github.com/Yuanbanba/Atmosphere/blob/main/launchmore.jpg" align="center" width="90%" />
