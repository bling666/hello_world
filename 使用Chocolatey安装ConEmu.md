### 使用Chocolatey安装ConEmu

在powershell利用Choco安装ConEmu时出现问题

显示安装失败

原因是某个文件夹拒绝了访问

解决方式是更改了powershell的脚本加载策略（或许）

1、输入 Get-ExecutionPolicy，结果可能为 Unrestricted、RemoteSigned、AllSigned、Restricted、Default、Bypass、Undefined ，分别对应某种策略

2、输入 Set-ExecutionPolicy Allsigned，将策略改为了Allsigned解决了问题。

详细查看有关ExectionPolicy的问题可以查看 

https://docs.microsoft.com/en-us/powershell/module/microsoft.powershell.security/set-executionpolicy?view=powershell-6 