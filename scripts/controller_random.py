#ID: randomctl
#TYPE: controller
#NAME: Dummy random controller

import env

class Controller:
    def __init__(self, team_id):
        self.team_id = team_id

    def initialize(self):
        count = env.control.get_robot_count(self.team_id)
        for i in range(count):
            env.control.set_robot_script(self.team_id, i, 'random')

    def update(self):
        pass
