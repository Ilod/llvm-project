set(LLVM_LINK_COMPONENTS
  Support
  )

add_lld_tool(lld
  lld.cpp
  )

target_link_libraries(lld
  lldDriver
  lldCOFF
  lldELF
  )

install(TARGETS lld
  RUNTIME DESTINATION bin)

if(NOT LLD_SYMLINKS_TO_CREATE)
  set(LLD_SYMLINKS_TO_CREATE lld-link ld.lld)
  
  if (WIN32)
    list(APPEND LLD_SYMLINKS_TO_CREATE ../msbuild-bin/link)
  endif()
endif()

foreach(link ${LLD_SYMLINKS_TO_CREATE})
  add_lld_symlink(${link} lld)
endforeach()
