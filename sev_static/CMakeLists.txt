
FILE(GLOB SRCS ../sev/*.cpp ../sev/impl/*.cpp)
FILE(GLOB HDRS ../sev/*.h ../sev/impl/*.h)
FILE(GLOB INLS ../sev/*.inl ../sev/impl/*.inl)

SOURCE_GROUP("" FILES ${SRCS} ${HDRS} ${INLS})

ADD_LIBRARY(sev_static
  ${SRCS}
  ${HDRS}
  ${INLS}
)

TARGET_LINK_LIBRARIES(sev_static
)

ADD_DEFINITIONS(-DSEV_CMAKE -DSEV_LIB_STATIC)
