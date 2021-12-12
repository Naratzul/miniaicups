import json
import click
import subprocess
import os
import time
import traceback
import multiprocessing
import tqdm
import shutil


def create_folder(folder):
    if not os.path.exists(folder):
        os.makedirs(folder)

    for filename in os.listdir(folder):
        file_path = os.path.join(folder, filename)
        try:
            if os.path.isfile(file_path) or os.path.islink(file_path):
                os.unlink(file_path)
            elif os.path.isdir(file_path):
                shutil.rmtree(file_path)
        except Exception as e:
            print(e.traceback())

def create_results_folder():
    create_folder("results")

def run_batch_game(idx):
    result_path = "result{idx}.txt".format(idx = idx)
    lr_command = "../build-local_runner-Desktop-Release/local_runner --save-results={result_path} --batch-mode".format(result_path = result_path)

    try:
        with subprocess.Popen(lr_command.split(' '), stdout = subprocess.DEVNULL, stderr = subprocess.DEVNULL) as local_runner:
            local_runner.wait()
    except Exception as e:
        print(e.traceback())

def run_game(seed):
    os.environ['SEED'] = seed

    lr_command = "../build-local_runner-Desktop-Release/local_runner"

    try:
        with subprocess.Popen(lr_command.split(' '), stdout = subprocess.DEVNULL, stderr = subprocess.DEVNULL) as local_runner:
            local_runner.wait()
    except Exception as e:
        print(e.traceback())

    del os.environ['SEED']

def process_result(game_count):
    player_count = 4;
    wins = 0
    loses = 0

    crash_seed = []
    loses_seed = []

    for result in os.listdir("results"):
        with open("results/{file}".format(file = result), "r") as result_file:
            result_json = json.load(result_file)
            seed = result_json['seed']

            if result_json['players'][0]['crashed']:
                crash_seed.append(seed)

            scores = []
            for i in range(player_count):
                scores.append(result_json['results'][i])

            sorted_scores = [i[0] for i in sorted(enumerate(scores), key = lambda x:x[1], reverse = True)]

            if sorted_scores[0] == 0:
                wins += 1
            else:
                loses += 1
                loses_seed.append(seed)

    print("Wins: {0}/{1}".format(wins, game_count))
    print("Loses: {0}/{1}".format(loses, game_count))
    print("WR: {0:.0%}".format(wins / game_count))

    if len(crash_seed) > 0:
        print("CRASHES")
        for crash in crash_seed:
            print(crash)

    if len(loses_seed) > 0:
        print("LOSES")
        for lose in loses_seed:
            print(lose)


def cleanup():
    shutil.rmtree("results")

def start_process():
    pass

def init(count, nthreads):
    create_results_folder()

    pool = multiprocessing.Pool(processes = nthreads, initializer = start_process)
    for _ in tqdm.tqdm(pool.imap_unordered(run_batch_game, [(idx) for idx in range(count)]), total = count):
        pass

    process_result(count)
    cleanup()

@click.group()
def main():
    pass

@main.command()
@click.option('--count', type = int, default = 10)
@click.option('--nthreads', type = int, default = 4)
def run(count, nthreads):
    init(count, nthreads)

@main.command()
@click.option('--seed')
def run_one(seed):
    run_game(seed)

if __name__ == "__main__":
    main()
