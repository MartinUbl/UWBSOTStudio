#ID: swing
#TYPE: agent
#NAME: Swinging movement

import env
import random

class Agent:
    def __init__(self, team_id, robot_id):
        self.team_id = team_id
        self.robot_id = robot_id

    def initialize(self):
        self.last_ms = -3000 # to start immediatelly
        self.state = 0
        self.next_timer = random.randint(2000, 3500)

    def update(self):
        if self.last_ms + self.next_timer < env.control.get_time_ms():
            self.last_ms = env.control.get_time_ms()
            self.next_timer = random.randint(2000, 3500)
            if self.state == 0:
                env.control.set_robot_speed(self.team_id, self.robot_id, 0.5, 0.5)
                self.state = 1
            else:
                env.control.set_robot_speed(self.team_id, self.robot_id, -0.5, -0.5)
                self.state = 0
