# Arduino 8266 Websocket
* 客户端通讯例子  

web地址：http://www.zhoti.com/witnet/Wsconnect/index  
ws通讯地址： ws.zhoti.com  
ws通讯端口： 9393  
  
  
  
  
# 功能  
* Web配网：
	> 设备上电即开启自身AP，默认密码是123456789，连上后192.168.4.1进入配网页面，配网之后会自动重启，默认AP关闭；  

* 获取通讯ID：  
	> 在串口或者路由器上可以看到配网之后的IP，访问http://IP/bindingcode 可以看到ws的通讯ID；  


> 如需重新配网，需要擦拭flash；  
> 编译固件时，src文件夹里面的ESP8266-Websocket文件夹复制到C:\Users\Administrator\Documents\Arduino\libraries文件夹里，就可以被引入了；  


# 使用  
* 获取到通讯ID，到web地址填入通讯ID，填入内容，点击发送，即可进行外网通讯；  
* 可以关注公众 门第 或者搜索 mdwxgzh   公众号会根据插件的扩展更新进行规则更变，如果不能使用，请回来看这里；  
	> 发送你的通讯ID+空格+指令，例：7f0000010bbb00000001 open_led