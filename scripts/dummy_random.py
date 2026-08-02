#ID: random
#TYPE: agent
#NAME: Random movement

import env
import random

class Agent:
    def __init__(self, team_id, robot_id):
        self.team_id = team_id
        self.robot_id = robot_id
        
    def initialize(self):
        self.last_ms = -2000 # to start immediatelly
        self.next_timer = random.randint(1000, 2500)

    def update(self):
        if self.last_ms + self.next_timer < env.control.get_time_ms():
            self.last_ms = env.control.get_time_ms()
            env.control.set_robot_speed(self.team_id, self.robot_id, random.random() - 0.5, random.random() - 0.5)
            self.next_timer = random.randint(1000, 2500)
