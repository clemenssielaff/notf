def initialize_logger():
    """
    Set up the system-wide logger.
    """
    import logging
    logging.basicConfig(format="[notf] %(asctime)s %(levelname)s: %(message)s", level=logging.DEBUG, datefmt="%H:%M:%S")


initialize_logger()
