class ApplicationLogic:
    pass


class LogicScheduler:
    pass


class Service:
    pass


class Fact:
    """
    notf Facts represent changeable, external truths that the application logic can react to.
    To the application logic they appear as simple Publishers that are owned and managed by a single Service in a
    thread-safe fashion. The service will update the Fact as new information becomes available, completes the Fact's
    Publisher when the Service is stopped or the Fact has expired (for example, when a sensor is disconnected) and
    forwards appropriate exceptions to the Subscribers of the Fact should an error occur.
    Examples of Facts are:
        - the latest reading of a sensor
        - the user performed a mouse click (incl. position, button and modifiers)
        - the complete chat log
        - the expiration signal of a timer
    Facts usually consist of a Value, but can also exist without one (in which case the None Value is used).
    Empty Fact act as a signal informing the logic that an event has occurred but without additional information.
    """
    pass
