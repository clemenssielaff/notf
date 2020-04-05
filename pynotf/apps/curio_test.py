from datetime import datetime
import curio
from random import randint, choice

messages = curio.Queue()
new_fibers = []
joinable_fibers = []
fiber_counter: int = 0


def print_msg(msg):
    print(f'{datetime.now().isoformat()}: {msg}')


def abc(msg):
    global fiber_counter
    fiber_counter += 1
    print_msg(f'Got {msg} -- starting new fiber {fiber_counter}')
    new_fibers.append((abc_fiber, fiber_counter))


async def abc_fiber(fiber_id):
    await curio.sleep(2)
    print_msg(f'Finished fiber {fiber_id}')
    joinable_fibers.append(await curio.current_task())


async def main_loop():
    active_fibers = set()
    while True:
        msg = await messages.get()
        print_msg(f'Event got {msg}')
        if msg in ('abc'):
            abc(msg)
        elif msg == 'x':
            for fiber in active_fibers:
                await fiber.join()
            return

        for fiber_args in new_fibers:
            active_fibers.add(await curio.spawn(*fiber_args))
        new_fibers.clear()

        for joinable_fiber in joinable_fibers:
            await joinable_fiber.join()
            assert joinable_fiber in active_fibers
            active_fibers.remove(joinable_fiber)
        joinable_fibers.clear()


async def produce_random_event(base_interval):
    while True:
        await curio.sleep(base_interval + (float(randint(-1000, 1000)) * base_interval / 1000.))
        # await messages.put(choice('abcdefghijklmnopqrstuvwxyz'))
        await messages.put(choice('abcdefghxyz'))


if __name__ == '__main__':
    async def run_main_and_event_producer():
        even_producer = await curio.spawn(produce_random_event, 1)
        main_task = await curio.spawn(main_loop)
        await main_task.join()
        await even_producer.cancel()


    curio.run(run_main_and_event_producer)
