#ID: dummy
#TYPE: controller
#NAME: Dummy chaotic controller

import env

class Controller:
    def __init__(self, team_id):
        self.team_id = team_id

    def initialize(self):
        count = env.control.get_robot_count()
        for i in range(int(count/2)):
            env.control.set_robot_script(self.team_id, i, 'swing')
        for i in range(int(count/2), int(count)):
            env.control.set_robot_script(self.team_id, i, 'random')

    def update(self):
        pass
