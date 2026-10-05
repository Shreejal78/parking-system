import random as r
import json
import os
filename = "rps.json"

moves = ["scissor","rock","paper"]
computer = r.choice(moves)[0].lower()
print(computer)

def loadScore():
    if(os.path.exists(filename)):
        with open(filename,"r") as f:
            loaded_data = json.load(f)
            print(f"Computer: {loaded_data["computer_score"]}       You: {loaded_data["player_score"]}")
    else:
        default_data = {
            "computer_score":0,            
            "player_score":0
        }
        with open(filename, "w") as f:            
            json.dump(default_data, f, indent=4)
        print(f"Computer: 0       You: 0")
    

def update_score(winner):
   
    with open(filename,"r") as f:
       loaded_data = json.load(f)
    if(winner == "computer"):
        loaded_data["computer_score"] += 1
    else:
        loaded_data["player_score"] += 1
    with open(filename,"w") as f:
        json.dump(loaded_data,f,indent=4)
        
def checkwin(usr,comp):
    if(usr == comp):
        return "<< DRAW >>\n"
    if(usr == "s" and comp == "r"):
        update_score("computer")
        return "<< COMPUTER WON >>\n"
    elif(usr == "r" and comp == "p"):
        update_score("computer")
        return "<< COMPUTER WON >>\n"
    elif(usr == "p" and comp == "s"):
        update_score("computer")
        return "<< COMPUTER WON >>\n"
    else:
        update_score("player")
        return "<< YOU WON >>\n"


def checkValidMove(move):
    if(move in moves):
        return True
    
def initGame():
    loadScore()
    print("[ROCK, PAPER, SCISSOR]")
    user = input('Enter your move:  ')[0].lower()
    while True:
        if(checkValidMove(user)):
            print(f"You: {user}") 
            print(f"Computer: {computer}")
            print(f"Result: {checkwin(user,computer)}")
            break
        else:
            user = input("Enter valid Move: ")

initGame()
while True:
    play = input("Press any Key to play Again. (Press E to exit)\n\n")
    if(play.lower() == "e"):
        break
    else:
        os.system("cls")
        initGame()
        