
DECLARE_HANDLE(RECORDER_LOG);
DECLARE_HANDLE(WPP_RECORDER_COUNTER);

#define RECORDER_LOG_IDENTIFIER_MAX_CHARS 16

typedef enum _WPP_RECORDER_TRI_STATE {
  WppRecorderFalse = FALSE,
  WppRecorderTrue = TRUE,
  WppRecorderUseDefault = 2
} WPP_RECORDER_TRI_STATE, *PWPP_RECORDER_TRI_STATE;

typedef struct _RECORDER_LOG_CREATE_PARAMS {
  ULONG                  Size;
  ULONG                  LogTag;
  ULONG                  TotalBufferSize;
  ULONG                  ErrorPartitionSize;
  ULONG_PTR              LogIdentifierAppendValue;
  BOOLEAN                LogIdentifierAppendValueSet;
  ULONG                  LogIdentifierSize;
  CHAR                   LogIdentifier[RECORDER_LOG_IDENTIFIER_MAX_CHARS];
  WPP_RECORDER_TRI_STATE UseTimeStamp;
  WPP_RECORDER_TRI_STATE PreciseTimeStamp;
} RECORDER_LOG_CREATE_PARAMS, *PRECORDER_LOG_CREATE_PARAMS;

typedef struct _RECORDER_CONFIGURE_PARAMS {
  ULONG                  Size;
  BOOLEAN                CreateDefaultLog;
  WPP_RECORDER_TRI_STATE UseTimeStamp;
  WPP_RECORDER_TRI_STATE PreciseTimeStamp;
} RECORDER_CONFIGURE_PARAMS, *PRECORDER_CONFIGURE_PARAMS;

typedef struct _WPP_TRIAGE_INFO {
  ULONG WppAutoLogHeaderSize;
  ULONG WppDriverContextOffset;
  ULONG WppAutoLogHeaderSizeOffset;
  ULONG WppSizeOfAutoLogHeaderSizeField;
  ULONG WppDriverContextSize;
} WPP_TRIAGE_INFO, *PWPP_TRIAGE_INFO;
