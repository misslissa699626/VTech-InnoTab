#!/usr/bin/env python

levels = ['FATAL', 'SEVERE', 'WARNING', 'INFO', 'FINE', 'FINER', 'FINEST']

def PrintDefine(x):
    print '#define '+x.replace(' ','').replace('#',' ')
    
for arg_count in xrange(10):
    if arg_count:
        arg_prefix  = str(arg_count)
        arg_prefix_ = '_'+arg_prefix
    else:
        arg_prefix  = ''
        arg_prefix_ = arg_prefix

    arg_list = ','.join(['_msg'] + ['_arg'+str(i) for i in range(1,arg_count+1)])
    arg_list_p = ','.join(['(_msg)'] + ['(_arg'+str(i)+')' for i in range(1,arg_count+1)])
    PrintDefine('ATX_LOG_LL%s(_logger, _level, %s)#ATX_LOG_X((_logger), (_level), ((_logger).logger, (_level), __FILE__, __LINE__, (ATX_LocalFunctionName), %s))' % (arg_prefix, arg_list, arg_list_p))
    PrintDefine('ATX_LOG%s   (         _level, %s)#ATX_LOG_LL%s((_ATX_LocalLogger), (_level), %s)' % (arg_prefix_, arg_list, arg_prefix, arg_list_p))
    PrintDefine('ATX_LOG_L%s (_logger, _level, %s)#ATX_LOG_LL%s((_logger),          (_level), %s)' % (arg_prefix,  arg_list, arg_prefix, arg_list_p))

print
for level in levels:
    for arg_count in xrange(10):
        if arg_count:
            arg_prefix  = str(arg_count)
            arg_prefix_ = '_'+arg_prefix
        else:
            arg_prefix  = ''
            arg_prefix_ = arg_prefix
        
        arg_list = ','.join(['_msg'] + ['_arg'+str(i) for i in range(1,arg_count+1)])
        arg_list_p = ','.join(['(_msg)'] + ['(_arg'+str(i)+')' for i in range(1,arg_count+1)])
        PrintDefine('ATX_LOG_%s%s   (         %s)#ATX_LOG_LL%s((_ATX_LocalLogger), ATX_LOG_LEVEL_%s, %s)' % (level, arg_prefix_, arg_list, arg_prefix, level, arg_list_p))
        PrintDefine('ATX_LOG_%s_L%s (_logger, %s)#ATX_LOG_LL%s((_logger),          ATX_LOG_LEVEL_%s, %s)' % (level, arg_prefix,  arg_list, arg_prefix, level, arg_list_p))
        
print
for level in levels:
    PrintDefine('ATX_CHECK_%s   (         _result)#ATX_CHECK_LL((_ATX_LocalLogger), ATX_LOG_LEVEL_%s, (_result))' % (level, level))
    PrintDefine('ATX_CHECK_%s_L (_logger, _result)#ATX_CHECK_LL((_logger),          ATX_LOG_LEVEL_%s, (_result))' % (level, level))
    
print
for level in levels:
    PrintDefine('ATX_CHECK_LABEL_%s   (         _result, _label)#ATX_CHECK_LABEL_LL((_ATX_LocalLogger), ATX_LOG_LEVEL_%s, (_result), _label)' % (level, level))
    PrintDefine('ATX_CHECK_LABEL_%s_L (_logger, _result, _label)#ATX_CHECK_LABEL_LL((_logger),          ATX_LOG_LEVEL_%s, (_result), _label)' % (level, level))



