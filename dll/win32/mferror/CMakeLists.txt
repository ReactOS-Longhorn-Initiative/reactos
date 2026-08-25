add_message_headers(UNICODE mferror_messages.mc)
add_library(mferror MODULE mferror_messages.rc)
set_module_type(mferror win32dll ENTRYPOINT 0)
add_dependencies(mferror mferror_messages)
