# Completed

- [x] Thriller question: needs to add an "as of" date

- [x] The trivia window has a message that says "click any key to continue" after the correct or wrong question is revealed. But right now the window is disappearing automatically.

- [x] After one block is placed, the selection highlight should automatically move to the next one in the queue (especially for controller and keyboard)

- [x] The default should snap the block to the middle of the play grid as the player moves the block toward there. 

- Lauren wanted the blocks to not be able to leave the play grid. She took note of the blocks turning red, but she was not happy about how the blocks were kind of rendered offscreen. In a recent update we realized this might not be the most helpful thing, but now the gameplay is better after the other improvements in this section

- [x] Important: the game should also turn the blocks red in the preview window (on the bottom) in a situation where it is impossible to place them on the board, this will get the player ready for the game over screen...

- [x] Playlist isn't switching to the next song

- [x] Enable the playlist to update dynamically when new files are added to the background music folder

- We streamlined the python helper scripts including creating an installable package. Now you can change the filenames to snake_case when converting, which has been the convention I've used so far (but I was changing the filenames myself, the horror!)

- bg music plays in a random order now, using a bagged random number generator that also ensures that a song doesn't play twice in a row when a new bag is ready

- New trivia questions added throughout

- Add executable icon (see exe-icon.md). https://game-icons.net/1x1/lorc/uncertainty.html#download Uncertainty icon by Lorc under CC BY 3.0

- We fixed the scaling behavior, so that the game looks nice when it is fullscreen

- Now the trivia-related stuff has been migrated to the new python package, promptukit

## Claude suggestions about overall workflow
- [ ] Commit a little CONTRIBUTING.md or docs/trivia-style-guide.md to the repo that captures the rules in human-readable form. Future-you (and any collaborators, including any future Claude session that needs to understand the project) will benefit from having the style explicit and version-controlled rather than living only inside a Claude Code skill file. The skill enforces it; the doc explains why. The "as of 2024" rule deserves a footnote explaining that it exists because of an Academy Awards Trivial Pursuit incident in (year), and that Lauren was right. Note: I have added new material to the audit-trivia.md file (and will add more as new ideas surface)

## Bugs with Partial Fixes

- [ ] Within the trivia questions and answers, some characters don't render properly, rendering a ? instead of another symbol. 

- [ ] Text also goes outside of the boxes sometimes; the quips should be able to word-wrap!

## Potential Enhancement

- [ ] We should reconsider how the 'streak' is used. I guess it resets after you've answered a 4th question in a row, but it shouldn't. Perhaps a streak of 3 could give you tokens that could be used to save you for getting a question wrong? and it could also save you from dying by allowing you to remove a few grid boxes of your choosing?

## Audio

- [ ] Add a mute button

## Feature enhancements



- [ ] Global leaderboard that defaults to local one with no internet connection. I have the logic built out in another Javascript game I made, and I have a Supabase database that can be used to store scores

## Bigger ideas

- [ ] Spin off the trivia creation into scripts that I can use to write test questions. We would create a new json file that would have a new question set, using the claude skills and python workflow we've already created. Then I could add a python script that would create an actual test file, adding the ability to do calculation questions.

- [ ] Make a packaged executable that is downloadable on github

- [ ] Investigate hosting on itch.io