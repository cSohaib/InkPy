# Stop and Close must work even when Python catches all exceptions.
while True:
    try:
        while True:
            pass
    except BaseException:
        pass
