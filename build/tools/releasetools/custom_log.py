#!/usr/bin/env python
# coding=utf8

import logging
import inspect

#*---------------------------------------------------------------------------*/

LOGGING_LEVEL=logging.DEBUG
# LOGGING_LEVEL=logging.INFO
# LOGGING_FMT='%(asctime)s %(levelname)-4s %(message)s'
LOGGING_FMT='%(levelname)-4s %(message)s'

logging.basicConfig(level=LOGGING_LEVEL,
                    format=LOGGING_FMT,
                    # format='asctime)s %(levelname)-4s %(message)s',
                    datefmt='%d %b %H:%M:%S')

# .KP : 
"""
配置是否使能 D(), 以及影响 I() 等方法的行为.
"""
# ENABLE_DEBUG_LOG = True
ENABLE_DEBUG_LOG = False

#*---------------------------------------------------------------------------*/

def __getBackFrameInfo(f):
  return f.f_back.f_code.co_filename, f.f_back.f_lineno, f.f_back.f_code.co_name


def __getAllArgsForLogging(fileName, lineNo, funcName, fmt, args):
  """
  返回元组形态的, 可作为 logging.debug() 等方法的实参列表.
  """
  return tuple( ['[file] : %s; [line] : %d : [func] : %s : ' + fmt, fileName, lineNo, funcName] + list(args) )


def D(fmt, *args):
  """
  带有源码位置信息的 debug log.
  """

  if ( not ENABLE_DEBUG_LOG ):
    return

  f = inspect.currentframe()
  # call_stack 中上一个 frame 的文件信息. 
  filename, lineno, funcName = __getBackFrameInfo(f)

  # 构建包含所有实参的 元组实例. 
  allArgs = __getAllArgsForLogging(filename, lineno, funcName, fmt, args)
  # print(allArgs)
  # print(len(allArgs) )
  # .KP : 使用内置函数 apply() 调用函数, 实参列表以元组形式传入. 
  apply(logging.debug, allArgs)

  # 另一种实现方式 : 使用 字串格式化操作符 "%" 的特性, 将元组作为 右操作数. 
  # logging.debug('[file] : %s; [line] : %d : %s', filename, lineno, fmt % args)


def I(fmt, *args):

  if ( not ENABLE_DEBUG_LOG ):
    print fmt % args
    return

  f = inspect.currentframe()
  filename, lineno, funcName = __getBackFrameInfo(f)

  allArgs = __getAllArgsForLogging(filename, lineno, funcName, fmt, args)
  apply(logging.info, allArgs)

  del f


def W(fmt, *args):

  if ( not ENABLE_DEBUG_LOG ):
    print fmt % args
    return

  f = inspect.currentframe()
  filename, lineno, funcName = __getBackFrameInfo(f)

  allArgs = __getAllArgsForLogging(filename, lineno, funcName, fmt, args)
  apply(logging.warning, allArgs)

  del f


def E(fmt, *args):

  if ( not ENABLE_DEBUG_LOG ):
    print fmt % args
    return

  f = inspect.currentframe()
  filename, lineno, funcName = __getBackFrameInfo(f)

  allArgs = __getAllArgsForLogging(filename, lineno, funcName, fmt, args)
  apply(logging.warning, allArgs)

  del f


def C(fmt, *args):

  if ( not ENABLE_DEBUG_LOG ):
    print fmt % args
    return

  f = inspect.currentframe()
  filename, lineno, funcName = __getBackFrameInfo(f)

  allArgs = tuple( ['[file] : %s; [line] : %d : ' + fmt, filename, lineno] + list(args) )
  apply(logging.critical, allArgs)

  del f

#*-------------------------------------------------------*#

def D_CS(fmt, *args):

  if ( not ENABLE_DEBUG_LOG ):
    return

  f = inspect.currentframe()
  filename, lineno, funcName = __getBackFrameInfo(f)
  del f

  # allArgs = tuple( ['[file] : %s; [line] : %d : [func] : %s : ' + fmt + ' : [call stack] : ', filename, lineno, funcName] + list(args) )
  allArgs = tuple( ['[file] : %s; [line] : %d : [func] : %s : ' + fmt , filename, lineno, funcName] + list(args) )
  apply(logging.debug, allArgs)
  logging.debug('[call stack] : ')

  stack=inspect.stack()
  for i in range(1, len(stack) ):       # "1" : stack[0] 是调用 stack() 处的 frame_record, 丢弃.
    filename = stack[i][1]
    lineno = stack[i][2]
    funcName = stack[i][3]
    code = stack[i][4][0]
    # logging.debug('\t[file] : %s; [line] : %d : [func] : %s; [code] : %s', filename, lineno, funcName, code[0 : -1] )
    logging.debug('\t\t<frame %d> : %s, line %d : %s', i - 1, filename, lineno, code[0 : -1] )


#*-------------------------------------------------------*#

def func1():
  # D("enter")
  D("I am %s : %s", "Aki", "enter")
  I("I am %s : %s", "Aki", "enter")
  W("I am %s : %s", "Aki", "enter")
  E("I am %s : %s", "Aki", "enter")
  C("I am %s : %s", "Aki", "enter")

  me = (1, 2, 3)
  D("me : " + str(me) )

  D_CS("%s", "here, to log call stack.")

#*-------------------------------------------------------*#

def main():
  # .T : Add Code ...

  func1()

  # print("%s", "done.")        # 将输出 : ('%s', 'done.')

  print( "%s" % "done.")
  # print "%s" % "done."        # 和上面的写法等价. 
  return

#*---------------------------------------------------------------------------*#
# 若当前源码被作为脚本执行, 则...
if __name__ == '__main__':
    main()


