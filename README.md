# TMQ
太合通用消息队列，为解决了现在各个业务线独立部署消息队列造成的服务器资源和人力维护成本浪费的问题，降低了业务接入消息队列的门槛。

## 架构设计
![](https://github.com/taihedeveloper/tmq/blob/master/design/tmq.png)
## 功能
* 支持配置热加载。
* 支持消息队列优雅退出。
* 支持消息队列永久阻塞和手动跳过（目前还未提交队列长度监控等脚本）。
* 支持消费方模板的消费方式消费（详情看使用消费模板一节）。
* 支持消费队列和消费方一对多的配置方式。
* 每个消费方以轮询的方式消费一个消费队列，具备高可用性、可扩展性等特性。
* TMQ任务的VHOST统一采用tmq。
* TMQ支持的生产者类型：TCP/MCPACK类型(MCPACK是内部类型，如使用可以理解为protobuf类型)和HTTP/JSON类型。
* TMQ支持的消费类型：MYSQL类型和HTTP类型。
* 添加TMQ消息队列，其中任务主键是队列的唯一标识，需要用户在mcpack包中或者json中协带参数command_no=$(bindkey)，否则无法找到相关的消费队列。http方式还需在url中携带common_no=xxx

## 使用消费模板

### 1. HTTP 消费模板
以http和json的消费为例：

 uri(http 模板，对应数据库标command_http_conf中的http_template字段): 
     ```
         /v1/restserver/ting?method=baidu.ting.ugcpush.pushInfo&transid={{#TRANSID}}&f=cm_transfer 
         ```

{{#TRANSID}} 为一个可变参数，这个参数在发送给producer中的json中需要携带

```
{"command_no":"100001", "TRANSID":100023}
```

### 2. MySQL 消费模板
以MySQL和json的消费为例：

 SQL:(SQL模板，对应数据库标command_db_conf中的db_template字段): 
     ```
         INSERT INTO user_info(username,email,tel_no)VALUES('$username$','$email$','$tel_no$')
         ```

         $username$ 为一个可变参数，可以看成一段文彬，这个参数在发送给producer中的json中需要携带

         ```
{"command_no":"100001", "username":"100023", "email":"xxxx@xx.com", "tel_no":"18888888888"}

## 后续
1.后续会添加对zookeeper的支持，以及监控消息队列长度等功能

## Copyright
taihedeveloper全体成员

