#INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR/httpcontext})

#NCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR/noncopyable})

INCLUDE_DIRECTORIES(/usr/locaincludel/)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

AUX_SOURCE_DIRECTORY(. DIR_SRCS)

ADD_EXECUTABLE(server ${DIR_SRCS} )

TARGET_LINK_LIBRARIES(server -pthread muduo_base sqlpool config log httpconn  timer -lmysqlclient) 
