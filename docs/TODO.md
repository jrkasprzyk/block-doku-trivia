# TODO

Based on a playtest on 4/11/2026, we started a TODO list. I don't know how to make checkboxes in markdown but perhaps we can add those later.

## Question validation and accuracy: specific questions that need patches
- [x] Thriller question: needs to add an "as of" date

## Claude suggestions about overall workflow
- [ ] Commit a little CONTRIBUTING.md or docs/trivia-style-guide.md to the repo that captures the rules in human-readable form. Future-you (and any collaborators, including any future Claude session that needs to understand the project) will benefit from having the style explicit and version-controlled rather than living only inside a Claude Code skill file. The skill enforces it; the doc explains why. The "as of 2024" rule deserves a footnote explaining that it exists because of an Academy Awards Trivial Pursuit incident in (year), and that Lauren was right. Note: I have added new material to the audit-trivia.md file (and will add more as new ideas surface)

## Bugs

- [x] The trivia window has a message that says "click any key to continue" after the correct or wrong question is revealed. But right now the window is disappearing automatically. 

- [ ] Within the trivia questions and answers, some characters don't render properly, rendering a ? instead of another symbol. 

- [ ] Text also goes outside of the boxes sometimes; the quips should be able to word-wrap!

## Drag and drop enhancements and fixes

- [ ] After one block is placed, the selection highlight should automatically move to the next one in the queue (especially for controller and keyboard)

- [ ] The default should snap the block to the middle of the play grid as the player moves the block toward there.

- [ ] Lauren wanted the blocks to not be able to leave the play grid. She took note of the blocks turning red, but she was not happy about how the blocks were kind of rendered offscreen. Let's see if this is achievable and if doing so would break other things

- [ ] Important: the game should also turn the blocks red in the preview window (on the bottom) in a situation where it is impossible to place them on the board, this will get the player ready for the game over screen...

## Audio

- [x] Playlist isn't switching to the next song

- [x] Enable the playlist to update dynamically when new files are added to the background music folder

- [ ] Add a mute button

## Feature enhancements

- [ ] Grab asset for executable icon (see exe-icon.md)

- [ ] Global leaderboard that defaults to local one with no internet connection. I have the logic built out in another Javascript game I made, and I have a Supabase database that can be used to store scores

## Bigger ideas

- [ ] Spin off the trivia creation into scripts that I can use to write test questions. We would create a new json file that would have a new question set, using the claude skills and python workflow we've already created. Then I could add a python script that would create an actual test file, adding the ability to do calculation questions.