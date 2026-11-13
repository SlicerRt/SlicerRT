import logging
import queue
import threading
from contextlib import contextmanager

#------------------------------------------------------------------------------
class LogForwardingHandler(logging.Handler):
  """ Forwards log messages to the log panel of the module.
      Messages from other threads are queued and forwarded from the main thread.
  """

  def __init__(self, sendMessage, level=logging.INFO):
    logging.Handler.__init__(self, level)
    self.sendMessage = sendMessage
    self.pendingMessages = queue.SimpleQueue()
    self.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))

  def emit(self, record):
    try:
      message = self.format(record)
    except Exception:
      self.handleError(record)
      return
    self.pendingMessages.put(message)
    if threading.current_thread() is threading.main_thread():
      self.flush()

  def flush(self):
    if threading.current_thread() is not threading.main_thread():
      return
    while True:
      try:
        message = self.pendingMessages.get_nowait()
      except queue.Empty:
        break
      self.sendMessage(message)

#------------------------------------------------------------------------------
@contextmanager
def forwardLogging(sendMessage, loggerNames, level=logging.INFO):
  handler = LogForwardingHandler(sendMessage, level)
  loggers = [logging.getLogger(name) for name in loggerNames]
  previousLevels = []
  for logger in loggers:
    previousLevels.append(logger.level)
    # Make sure records at the requested level are not filtered out before reaching the handler
    if logger.getEffectiveLevel() > level:
      logger.setLevel(level)
    logger.addHandler(handler)
  try:
    yield handler
  finally:
    for logger, previousLevel in zip(loggers, previousLevels):
      logger.removeHandler(handler)
      logger.setLevel(previousLevel)
    # Forward messages queued by worker threads after the last main-thread record
    handler.flush()
