def initialize_logger():
    """
    Set up the system-wide logger.
    """
    import logging
    logging.basicConfig(format="[notf] %(asctime)s %(levelname)s:\t%(message)s", level=logging.DEBUG)


initialize_logger()
