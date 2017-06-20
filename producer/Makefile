#COMAKE2 edit-mode: -*- Makefile -*-
####################64Bit Mode####################
ifeq ($(shell uname -m),x86_64)
CC=gcc
CXX=g++
CXXFLAGS=-g \
  -pipe \
  -W \
  -O0 \
  -Wall \
  -fPIC \
  -DDEBUG
CFLAGS=-g \
  -pipe \
  -W \
  -O0 \
  -Wall \
  -fPIC \
  -DDEBUG
CPPFLAGS=-D_GNU_SOURCE \
  -D__STDC_LIMIT_MACROS
DEPENDENCY_PATH=../../
BOOST_PATH=$(DEPENDENCY_PATH)third-64/boost
OPENSSL_PATH=$(DEPENDENCY_PATH)third-64/openssl
INCPATH=-I./
DEP_INCPATH=-I$(BOOST_PATH) \
  -I$(BOOST_PATH)/include \
  -I$(BOOST_PATH)/output \
  -I$(BOOST_PATH)/output/include \
  -I$(OPENSSL_PATH)/include \
  -I$(OPENSSL_PATH)/include/openssl \
  -I$(DEPENDENCY_PATH)lib2-64/ullib \
  -I$(DEPENDENCY_PATH)lib2-64/ullib/include \
  -I$(DEPENDENCY_PATH)lib2-64/ullib/output \
  -I$(DEPENDENCY_PATH)lib2-64/ullib/output/include \
  -I$(DEPENDENCY_PATH)lib2-64/bsl \
  -I$(DEPENDENCY_PATH)lib2-64/bsl/include \
  -I$(DEPENDENCY_PATH)lib2-64/bsl/output \
  -I$(DEPENDENCY_PATH)lib2-64/bsl/output/include \
  -I$(DEPENDENCY_PATH)public/comlog-plugin \
  -I$(DEPENDENCY_PATH)public/comlog-plugin/include \
  -I$(DEPENDENCY_PATH)public/comlog-plugin/output \
  -I$(DEPENDENCY_PATH)public/comlog-plugin/output/include \
  -I$(DEPENDENCY_PATH)public/configure \
  -I$(DEPENDENCY_PATH)public/configure/include \
  -I$(DEPENDENCY_PATH)public/configure/output \
  -I$(DEPENDENCY_PATH)public/configure/output/include \
  -I$(DEPENDENCY_PATH)public/mcpack \
  -I$(DEPENDENCY_PATH)public/mcpack/output \
  -I$(DEPENDENCY_PATH)public/mcpack/output/include \
  -I$(DEPENDENCY_PATH)public/nshead \
  -I$(DEPENDENCY_PATH)public/nshead/output/include \
  -I$(DEPENDENCY_PATH)third-64/rabbitmq \
  -I$(DEPENDENCY_PATH)third-64/rabbitmq/include \
  -I$(DEPENDENCY_PATH)third-64/zookeeper \
  -I$(DEPENDENCY_PATH)third-64/zookeeper/include \
  -I$(DEPENDENCY_PATH)third-64/zookeeper/include/zookeeper \
  -I$(DEPENDENCY_PATH)third-64/libevent \
  -I$(DEPENDENCY_PATH)third-64/libevent/include \
  -I../common-net-app-lib/output/include

#============ CCP vars ============
CCHECK_FLAGS=
PCLINT=@pclint
PCLINT_FLAGS=
CCP=@ccp.py
CCP_FLAGS=

mqserver_producer_obj=mqserver_producer.o \
                      rabbitmq_producer.o \
                      libev_http_server.o

.PHONY:all
all: mqserver_producer
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mall[0m']"
	@echo "make all done"

.PHONY:clean
clean:
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mclean[0m']"
	rm -rf mqserver_producer
	rm -rf *.o

mqserver_producer:$(mqserver_producer_obj)
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mmqserver_producer[0m']"
	$(CXX) $(mqserver_producer_obj) \
    -Xlinker "-(" \
    $(BOOST_PATH)/lib/libboost_atomic.a \
    $(BOOST_PATH)/lib/libboost_chrono.a \
    $(BOOST_PATH)/lib/libboost_container.a \
    $(BOOST_PATH)/lib/libboost_context.a \
    $(BOOST_PATH)/lib/libboost_coroutine.a \
    $(BOOST_PATH)/lib/libboost_date_time.a \
    $(BOOST_PATH)/lib/libboost_exception.a \
    $(BOOST_PATH)/lib/libboost_filesystem.a \
    $(BOOST_PATH)/lib/libboost_graph.a \
    $(BOOST_PATH)/lib/libboost_locale.a \
    $(BOOST_PATH)/lib/libboost_log_setup.a \
    $(BOOST_PATH)/lib/libboost_math_c99.a \
    $(BOOST_PATH)/lib/libboost_math_c99f.a \
    $(BOOST_PATH)/lib/libboost_math_c99l.a \
    $(BOOST_PATH)/lib/libboost_math_tr1.a \
    $(BOOST_PATH)/lib/libboost_math_tr1f.a \
    $(BOOST_PATH)/lib/libboost_math_tr1l.a \
    $(BOOST_PATH)/lib/libboost_prg_exec_monitor.a \
    $(BOOST_PATH)/lib/libboost_program_options.a \
    $(BOOST_PATH)/lib/libboost_python.a \
    $(BOOST_PATH)/lib/libboost_random.a \
    $(BOOST_PATH)/lib/libboost_regex.a \
    $(BOOST_PATH)/lib/libboost_serialization.a \
    $(BOOST_PATH)/lib/libboost_signals.a \
    $(BOOST_PATH)/lib/libboost_system.a \
    $(BOOST_PATH)/lib/libboost_test_exec_monitor.a \
    $(BOOST_PATH)/lib/libboost_thread.a \
    $(BOOST_PATH)/lib/libboost_timer.a \
    $(BOOST_PATH)/lib/libboost_unit_test_framework.a \
    $(BOOST_PATH)/lib/libboost_wave.a \
    $(BOOST_PATH)/lib/libboost_wserialization.a \
    $(OPENSSL_PATH)/lib/libcrypto.a \
    $(OPENSSL_PATH)/lib/libssl.a \
    $(DEPENDENCY_PATH)lib2-64/ullib/lib/libullib.a \
    $(DEPENDENCY_PATH)lib2-64/bsl/lib/libbsl.a \
    $(DEPENDENCY_PATH)lib2-64/bsl/lib/libbsl_ResourcePool.a \
    $(DEPENDENCY_PATH)lib2-64/bsl/lib/libbsl_archive.a \
    $(DEPENDENCY_PATH)lib2-64/bsl/lib/libbsl_buffer.a \
    $(DEPENDENCY_PATH)lib2-64/bsl/lib/libbsl_check_cast.a \
    $(DEPENDENCY_PATH)lib2-64/bsl/lib/libbsl_exception.a \
    $(DEPENDENCY_PATH)lib2-64/bsl/lib/libbsl_pool.a \
    $(DEPENDENCY_PATH)lib2-64/bsl/lib/libbsl_utils.a \
    $(DEPENDENCY_PATH)lib2-64/bsl/lib/libbsl_var.a \
    $(DEPENDENCY_PATH)lib2-64/bsl/lib/libbsl_var_implement.a \
    $(DEPENDENCY_PATH)lib2-64/bsl/lib/libbsl_var_utils.a \
    $(DEPENDENCY_PATH)public/comlog-plugin/libcomlog.a \
    $(DEPENDENCY_PATH)public/comlog-plugin/output/lib/libdfsappender.a \
    $(DEPENDENCY_PATH)public/configure/libconfig.a \
    $(DEPENDENCY_PATH)public/spreg/libspreg.a \
    $(DEPENDENCY_PATH)public/mcpack/output/lib/libmcpack.a \
    $(DEPENDENCY_PATH)public/nshead/output/libnshead.a \
    $(DEPENDENCY_PATH)third-64/rabbitmq/lib64/librabbitmq.a \
    $(DEPENDENCY_PATH)third-64/zookeeper/lib/libzookeeper_mt.a \
    $(DEPENDENCY_PATH)third-64/zookeeper/lib/libzookeeper_st.a \
    $(DEPENDENCY_PATH)third-64/libevent/lib/libevent.a \
    $(DEPENDENCY_PATH)third-64/libevent/lib/libevent_core.a \
    $(DEPENDENCY_PATH)third-64/libevent/lib/libevent_extra.a \
    $(DEPENDENCY_PATH)third-64/libevent/lib/libevent_openssl.a \
    $(DEPENDENCY_PATH)third-64/libevent/lib/libevent_pthreads.a \
    ../common-net-app-lib/output/lib/libcommon_net.a \
    -lpthread -lcrypto -lrt -lz \
    -rdynamic -Xlinker "-)" -g -o mqserver_producer

mqserver_producer.o:mqserver_producer.cpp
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mmqserver_producer.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS) -o mqserver_producer.o mqserver_producer.cpp

rabbitmq_producer.o:rabbitmq_producer.cpp
	@echo "[^[[1;32;40mMAKE:BUILD^[[0m][Target:'^[[1;32;40mrabbitmq_producer.o^[[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS) -o rabbitmq_producer.o rabbitmq_producer.cpp

libev_http_server.o:libev_http_server.cpp
	@echo "[^[[1;32;40mMAKE:BUILD^[[0m][Target:'^[[1;32;40mlibev_http_server.o^[[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS) -o libev_http_server.o libev_http_server.cpp

endif #ifeq ($(shell uname -m),x86_64)
