#encoding=utf-8

import sys
import os
import time
import getopt
import urllib2

CONFIG_FILE_NAME = 'CODEMAKELIST'

#读取配置文件
def read_config_file():
    config_datas = {}
    config_path = os.path.split(os.path.realpath(__file__))[0]
    config_path = config_path.rstrip(' \t/\\')
    config_file = config_path +"/" + CONFIG_FILE_NAME
    if not os.path.exists(config_file):
        print config_file + " is not exist!"
        return config_datas
    fi = open(config_file, "r")
    lines = fi.readlines()
    key = ''
    last_key = ''
    item = []
    for line in lines:
        line = line.strip(' \t\r\n')
        if line.startswith('#') or len(line) == 0:
            continue
        if line.startswith('['):
            if not line.endswith(']'):
                print "config file line: " + line + " invalid format"
                sys.exit(1)
            if len(last_key) > 0:
                if last_key not in config_datas:
                    config_datas[key] = []
                if len(item) > 0:
                    config_datas[key].append(item)
            key = line.strip('[]')
            key = key.strip(' \t\r\n')
            last_key = key
            item = []
            continue
        if line.find('=') < 0 or len(key) == 0:
            print "config file line: " + line + " invalid format"
            sys.exit(1)
        item.append(line)
    if len(last_key) > 0:
        if last_key not in config_datas:
            config_datas[key] = []
        if len(item) > 0:
            config_datas[key].append(item)
    return config_datas

#解析配置数据
def parse_config_data(config_datas):
    work_root_key = "work_root"
    svn_root_key = "svn_root"
    library_key = "library"
    if work_root_key not in config_datas or len(config_datas[work_root_key]) != 1:
        print work_root_key + " is not exist or more then one!"
        sys.exit(1)
    if svn_root_key not in config_datas or len(config_datas[svn_root_key]) != 1:
        print svn_root_key + " is not exist or more then one!"
        sys.exit(1)
    if library_key not in config_datas:
        print library_key + " is not exist!"
        sys.exit(1)
    work_root_list = config_datas[work_root_key][0]
    work_root_path = ''
    for item in work_root_list:
        item = item.strip(' \t\r\n')
        if item.startswith('path'):
            item = item[4:]
            work_root_path = item.strip(' \t\r\n=')
    if len(work_root_path) == 0:
        print work_root_key + " is not exist!"
        sys.exit(1)
    svn_root_list = config_datas[svn_root_key][0]
    svn_root_path = ''
    for item in svn_root_list:
        item = item.strip(' \t\r\n')
        if item.startswith('path'):
            item = item[4:]
            svn_root_path = item.strip(' \t\r\n=')
    if len(svn_root_path) == 0:
        print svn_root_key + " is not exist!"
        sys.exit(1)
    library_list = config_datas[library_key]
    config_infos = []
    for librarys in library_list:
        item = {}
        item['local'] = ''
        item['svn'] = ''
        item['cmd'] = ''
        for lib in librarys:
            lib = lib.strip(' \t\r\n')
            if lib.startswith('local'):
                lib = lib[5:]
                item['local'] = work_root_path + lib.strip(' \t\r\n=')
            elif lib.startswith('svn'):
                lib = lib[3:]
                item['svn'] = svn_root_path + lib.strip(' \t\r\n=')
            elif lib.startswith('cmd'):
                lib = lib[3:]
                item['cmd'] = lib.strip(' \t\r\n=')
        if len(item['local']) == 0 or len(item['svn']) == 0 or len(item['cmd']) == 0:
            print "library item is error!"
            sys.exit(1)
        config_infos.append(item)
    return config_infos

#从svn上拉取源代码
def update_from_svn(svn_path, local_path):
    revision = 0
    cmd = "rm -rf %s" % (local_path)
    os.system(cmd)
    cmd = "svn checkout %s " % (svn_path) + local_path
    os.system(cmd)
    if os.path.exists(local_path) == False:
        e_str = '%s %s check out Error: %s' % (os.path.basename(__file__), sys._getframe().f_lineno, cmd)
        print e_str
        sys.exit(1)

#编译
def build(svn_path, local_path, cmd):
    make_file1 = ""
    make_file2 = "xxx"
    if "make" in cmd:
        make_file1 = "%s/Makefile" % (local_path)
        make_file2 = "%s/makefile" % (local_path)
    elif "build.sh" in cmd:
        make_file1 = "%s/build.sh" % (local_path)
        if cmd[0:8] == "build.sh":
            cmd = "./" + cmd
        if os.path.exists(make_file1):
            command = "chmod +x %s" % (make_file1)
            os.system(command)
    else:
        e_str = '%s %s %s not support, should be make or sh build.sh' % (os.path.basename(__file__), sys._getframe().f_lineno, cmd)
        print e_str
        sys.exit(1)
    if not os.path.exists(make_file1) and not os.path.exists(make_file2):
        e_str = '%s %s %s has no Makefile or build.sh' % (os.path.basename(__file__), sys._getframe().f_lineno, svn_path)
        print e_str
    command = "cd %s && make clean" % (local_path)
    os.system(command)
    command = "cd %s && %s" % (local_path, cmd)
    os.system(command)

def make_code(is_update, is_build):
    config_datas = read_config_file()
    config_infos = parse_config_data(config_datas)
    if is_update:
        for item in config_infos:
            update_from_svn(item['svn'], item['local'])
    if is_build:
        for item in config_infos:
            build(item['svn'], item['local'], item['cmd'])

#u：update，即从svn更新下载
#b：build，即进行编译
if __name__ == '__main__':
    is_relay = False
    is_update = False
    is_build = False
    try:
        opts, args = getopt.getopt(sys.argv[1:], "ub", ["update","build"])
        if len(opts) == 0:
            print "options is null"
            print("Usage:%s [-u|-b]|[--update|--build] args...." % sys.argv[0]);
            sys.exit(1)
        for opt, arg in opts:
            if opt in ("-u", "--update"):
                is_update = True
            elif opt in ("-b", "--build"):
                is_build = True
    except getopt.GetoptError, e:
        e_str = '%s %s %s' % (os.path.basename(__file__), sys._getframe().f_lineno, str(e))
        print e_str
        sys.exit(1)
    make_code(is_update, is_build)
