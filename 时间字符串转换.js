//在处理后端传给的时间是发现格式是2019-11-25T09:07:12.768Z的形式
//在网上找了找后发现这种带有T和Z的字符串叫做ISO8601字符串日期
//转换成正常日期的方法如下所示

function formDate(dateForm) {
                if (dateForm === "") {  //解决deteForm为空传1970-01-01 00:00:00
                    return "";
                }else{
                    var dateee = new Date(dateForm ).toJSON();
                    var date = new Date(+new Date(dateee)+ 8 * 3600 * 1000).toISOString().replace(/T/g,' ').replace(/\.[\d]{3}Z/,'');
                    return date;
                }
            }
/*
————————————————
版权声明：本文为CSDN博主「fanfange」的原创文章，遵循 CC 4.0 BY-SA 版权协议，转载请附上原文出处链接及本声明。
原文链接：https://blog.csdn.net/weixin_42481234/article/details/88566334
*/