#ifndef _BAZ_RESPONSESGITHUB_HPP
#define _BAZ_RESPONSESGITHUB_HPP
#include <string>

namespace ResponsesGitHub
{
    std::string alphaReleaseOnlyResponse = {R"alphaReleaseOnly([{"url":"https://api.github.com/repos/karusb/BazPO/releases/68102527","assets_url":"https://api.github.com/repos/karusb/BazPO/releases/68102527/assets","upload_url":"https://uploads.github.com/repos/karusb/BazPO/releases/68102527/assets{?name,label}","html_url":"https://github.com/karusb/BazPO/releases/tag/v0.2.1","id":68102527,"author":{"login":"karusb","id":15250436,"node_id":"MDQ6VXNlcjE1MjUwNDM2","avatar_url":"https://avatars.githubusercontent.com/u/15250436?v=4","gravatar_id":"","url":"https://api.github.com/users/karusb","html_url":"https://github.com/karusb","followers_url":"https://api.github.com/users/karusb/followers","following_url":"https://api.github.com/users/karusb/following{/other_user}","gists_url":"https://api.github.com/users/karusb/gists{/gist_id}","starred_url":"https://api.github.com/users/karusb/starred{/owner}{/repo}","subscriptions_url":"https://api.github.com/users/karusb/subscriptions","organizations_url":"https://api.github.com/users/karusb/orgs","repos_url":"https://api.github.com/users/karusb/repos","events_url":"https://api.github.com/users/karusb/events{/privacy}","received_events_url":"https://api.github.com/users/karusb/received_events","type":"User","site_admin":false},"node_id":"RE_kwDOHBsIgc4EDyl_","tag_name":"v0.2.1","target_commitish":"master","name":"BazPO v0.2.1","draft":false,"prerelease":true,"created_at":"2022-05-29T20:47:31Z","published_at":"2022-05-29T20:50:56Z","assets":[],"tarball_url":"https://api.github.com/repos/karusb/BazPO/tarball/v0.2.1","zipball_url":"https://api.github.com/repos/karusb/BazPO/zipball/v0.2.1","body":"Fixes\r\n- Fix asking for user input when one of the parameters is already provided for MutuallyExclusive options"},{"url":"https://api.github.com/repos/karusb/BazPO/releases/68079576","assets_url":"https://api.github.com/repos/karusb/BazPO/releases/68079576/assets","upload_url":"https://uploads.github.com/repos/karusb/BazPO/releases/68079576/assets{?name,label}","html_url":"https://github.com/karusb/BazPO/releases/tag/v0.2.0","id":68079576,"author":{"login":"karusb","id":15250436,"node_id":"MDQ6VXNlcjE1MjUwNDM2","avatar_url":"https://avatars.githubusercontent.com/u/15250436?v=4","gravatar_id":"","url":"https://api.github.com/users/karusb","html_url":"https://github.com/karusb","followers_url":"https://api.github.com/users/karusb/followers","following_url":"https://api.github.com/users/karusb/following{/other_user}","gists_url":"https://api.github.com/users/karusb/gists{/gist_id}","starred_url":"https://api.github.com/users/karusb/starred{/owner}{/repo}","subscriptions_url":"https://api.github.com/users/karusb/subscriptions","organizations_url":"https://api.github.com/users/karusb/orgs","repos_url":"https://api.github.com/users/karusb/repos","events_url":"https://api.github.com/users/karusb/events{/privacy}","received_events_url":"https://api.github.com/users/karusb/received_events","type":"User","site_admin":false},"node_id":"RE_kwDOHBsIgc4EDs_Y","tag_name":"v0.2.0","target_commitish":"master","name":"BazPO v0.2.0","draft":false,"prerelease":true,"created_at":"2022-05-28T21:30:58Z","published_at":"2022-05-28T21:49:41Z","assets":[],"tarball_url":"https://api.github.com/repos/karusb/BazPO/tarball/v0.2.0","zipball_url":"https://api.github.com/repos/karusb/BazPO/zipball/v0.2.0","body":"**Interface changes**\r\n\r\n- EitherMandatory renamed to MutuallyExclusive.\r\n- add interface separated into option, flag and tagless\r\n\r\n**Feature Updates**\r\n\r\n- TaglessOption can now be combined with other options\r\n- Support multiple constraints for one option\r\n\r\n**Changes**\r\n\r\n- MutuallyExclusive no longer makes the passed options mandatory.\r\n- By default, all options accept the maximum amount of values\r\n\r\n**Fixes**\r\n\r\n- Fixed when the user was asked input, reading the value would cause invalid pointer dereference."},{"url":"https://api.github.com/repos/karusb/BazPO/releases/67834323","assets_url":"https://api.github.com/repos/karusb/BazPO/releases/67834323/assets","upload_url":"https://uploads.github.com/repos/karusb/BazPO/releases/67834323/assets{?name,label}","html_url":"https://github.com/karusb/BazPO/releases/tag/v0.1.1","id":67834323,"author":{"login":"karusb","id":15250436,"node_id":"MDQ6VXNlcjE1MjUwNDM2","avatar_url":"https://avatars.githubusercontent.com/u/15250436?v=4","gravatar_id":"","url":"https://api.github.com/users/karusb","html_url":"https://github.com/karusb","followers_url":"https://api.github.com/users/karusb/followers","following_url":"https://api.github.com/users/karusb/following{/other_user}","gists_url":"https://api.github.com/users/karusb/gists{/gist_id}","starred_url":"https://api.github.com/users/karusb/starred{/owner}{/repo}","subscriptions_url":"https://api.github.com/users/karusb/subscriptions","organizations_url":"https://api.github.com/users/karusb/orgs","repos_url":"https://api.github.com/users/karusb/repos","events_url":"https://api.github.com/users/karusb/events{/privacy}","received_events_url":"https://api.github.com/users/karusb/received_events","type":"User","site_admin":false},"node_id":"RE_kwDOHBsIgc4ECxHT","tag_name":"v0.1.1","target_commitish":"master","name":"BazPO v0.1.1","draft":false,"prerelease":true,"created_at":"2022-05-25T19:06:53Z","published_at":"2022-05-25T19:13:43Z","assets":[],"tarball_url":"https://api.github.com/repos/karusb/BazPO/tarball/v0.1.1","zipball_url":"https://api.github.com/repos/karusb/BazPO/zipball/v0.1.1","body":"- Fixed EitherMandatory user input\r\n- Added FunctionConstraint to Cli\r\n- Printed text improvements"},{"url":"https://api.github.com/repos/karusb/BazPO/releases/66320413","assets_url":"https://api.github.com/repos/karusb/BazPO/releases/66320413/assets","upload_url":"https://uploads.github.com/repos/karusb/BazPO/releases/66320413/assets{?name,label}","html_url":"https://github.com/karusb/BazPO/releases/tag/v0.1.0","id":66320413,"author":{"login":"karusb","id":15250436,"node_id":"MDQ6VXNlcjE1MjUwNDM2","avatar_url":"https://avatars.githubusercontent.com/u/15250436?v=4","gravatar_id":"","url":"https://api.github.com/users/karusb","html_url":"https://github.com/karusb","followers_url":"https://api.github.com/users/karusb/followers","following_url":"https://api.github.com/users/karusb/following{/other_user}","gists_url":"https://api.github.com/users/karusb/gists{/gist_id}","starred_url":"https://api.github.com/users/karusb/starred{/owner}{/repo}","subscriptions_url":"https://api.github.com/users/karusb/subscriptions","organizations_url":"https://api.github.com/users/karusb/orgs","repos_url":"https://api.github.com/users/karusb/repos","events_url":"https://api.github.com/users/karusb/events{/privacy}","received_events_url":"https://api.github.com/users/karusb/received_events","type":"User","site_admin":false},"node_id":"RE_kwDOHBsIgc4D8_gd","tag_name":"v0.1.0","target_commitish":"master","name":"BazPO v0.1.0","draft":false,"prerelease":true,"created_at":"2022-05-08T18:28:17Z","published_at":"2022-05-08T18:33:06Z","assets":[],"tarball_url":"https://api.github.com/repos/karusb/BazPO/tarball/v0.1.0","zipball_url":"https://api.github.com/repos/karusb/BazPO/zipball/v0.1.0","body":"Added Constraints\r\nSome syntax changes"},{"url":"https://api.github.com/repos/karusb/BazPO/releases/65011847","assets_url":"https://api.github.com/repos/karusb/BazPO/releases/65011847/assets","upload_url":"https://uploads.github.com/repos/karusb/BazPO/releases/65011847/assets{?name,label}","html_url":"https://github.com/karusb/BazPO/releases/tag/v0.0.1","id":65011847,"author":{"login":"karusb","id":15250436,"node_id":"MDQ6VXNlcjE1MjUwNDM2","avatar_url":"https://avatars.githubusercontent.com/u/15250436?v=4","gravatar_id":"","url":"https://api.github.com/users/karusb","html_url":"https://github.com/karusb","followers_url":"https://api.github.com/users/karusb/followers","following_url":"https://api.github.com/users/karusb/following{/other_user}","gists_url":"https://api.github.com/users/karusb/gists{/gist_id}","starred_url":"https://api.github.com/users/karusb/starred{/owner}{/repo}","subscriptions_url":"https://api.github.com/users/karusb/subscriptions","organizations_url":"https://api.github.com/users/karusb/orgs","repos_url":"https://api.github.com/users/karusb/repos","events_url":"https://api.github.com/users/karusb/events{/privacy}","received_events_url":"https://api.github.com/users/karusb/received_events","type":"User","site_admin":false},"node_id":"RE_kwDOHBsIgc4D4ACH","tag_name":"v0.0.1","target_commitish":"master","name":"BazPO v0.0.1","draft":false,"prerelease":true,"created_at":"2022-04-21T20:46:45Z","published_at":"2022-04-21T20:50:11Z","assets":[],"tarball_url":"https://api.github.com/repos/karusb/BazPO/tarball/v0.0.1","zipball_url":"https://api.github.com/repos/karusb/BazPO/zipball/v0.0.1","body":"### **BazPO Pre-Release**"}])alphaReleaseOnly"};

    std::vector<Release> alphaReleaseOnly {
    {"v0.2.1", "BazPO v0.2.1", {},
    {"https://api.github.com/repos/karusb/BazPO/tarball/v0.2.1", "v0.2.1.tar.gz",},
    {"https://api.github.com/repos/karusb/BazPO/zipball/v0.2.1", "v0.2.1.zip"}, 
    true, false},
    {"v0.2.0", "BazPO v0.2.0", {},
    {"https://api.github.com/repos/karusb/BazPO/tarball/v0.2.0", "v0.2.0.tar.gz",},
    {"https://api.github.com/repos/karusb/BazPO/zipball/v0.2.0", "v0.2.0.zip"}, 
    true, false},
    {"v0.1.1", "BazPO v0.1.1", {},
    {"https://api.github.com/repos/karusb/BazPO/tarball/v0.1.1", "v0.1.1.tar.gz",},
    {"https://api.github.com/repos/karusb/BazPO/zipball/v0.1.1", "v0.1.1.zip"}, 
    true, false},
    {"v0.1.0", "BazPO v0.1.0", {},
    {"https://api.github.com/repos/karusb/BazPO/tarball/v0.1.0", "v0.1.0.tar.gz",},
    {"https://api.github.com/repos/karusb/BazPO/zipball/v0.1.0", "v0.1.0.zip"}, 
    true, false},
    {"v0.0.1", "BazPO v0.0.1", {},
    {"https://api.github.com/repos/karusb/BazPO/tarball/v0.0.1", "v0.0.1.tar.gz",},
    {"https://api.github.com/repos/karusb/BazPO/zipball/v0.0.1", "v0.0.1.zip"}, 
    true, false}};

    std::string latestReleaseNotFound = {R"latestRelease({"message":"Not Found","documentation_url":"https://docs.github.com/rest/releases/releases#get-the-latest-release"})latestRelease"};

    std::string mixedReleasesResponse = {R"mixedReleases([{"url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/releases/41605006","assets_url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/releases/41605006/assets","upload_url":"https://uploads.github.com/repos/karusb/FLF-FindLargeFolders/releases/41605006/assets{?name,label}","html_url":"https://github.com/karusb/FLF-FindLargeFolders/releases/tag/2.2","id":41605006,"author":{"login":"karusb","id":15250436,"node_id":"MDQ6VXNlcjE1MjUwNDM2","avatar_url":"https://avatars.githubusercontent.com/u/15250436?v=4","gravatar_id":"","url":"https://api.github.com/users/karusb","html_url":"https://github.com/karusb","followers_url":"https://api.github.com/users/karusb/followers","following_url":"https://api.github.com/users/karusb/following{/other_user}","gists_url":"https://api.github.com/users/karusb/gists{/gist_id}","starred_url":"https://api.github.com/users/karusb/starred{/owner}{/repo}","subscriptions_url":"https://api.github.com/users/karusb/subscriptions","organizations_url":"https://api.github.com/users/karusb/orgs","repos_url":"https://api.github.com/users/karusb/repos","events_url":"https://api.github.com/users/karusb/events{/privacy}","received_events_url":"https://api.github.com/users/karusb/received_events","type":"User","site_admin":false},"node_id":"MDc6UmVsZWFzZTQxNjA1MDA2","tag_name":"2.2","target_commitish":"master","name":"BazWare Find Large Folders v2.2","draft":false,"prerelease":false,"created_at":"2021-04-18T12:50:36Z","published_at":"2021-04-18T12:52:08Z","assets":[{"url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/releases/assets/35317536","id":35317536,"node_id":"MDEyOlJlbGVhc2VBc3NldDM1MzE3NTM2","name":"BazWareFindLargeFolders-2.2-Installer.zip","label":null,"uploader":{"login":"karusb","id":15250436,"node_id":"MDQ6VXNlcjE1MjUwNDM2","avatar_url":"https://avatars.githubusercontent.com/u/15250436?v=4","gravatar_id":"","url":"https://api.github.com/users/karusb","html_url":"https://github.com/karusb","followers_url":"https://api.github.com/users/karusb/followers","following_url":"https://api.github.com/users/karusb/following{/other_user}","gists_url":"https://api.github.com/users/karusb/gists{/gist_id}","starred_url":"https://api.github.com/users/karusb/starred{/owner}{/repo}","subscriptions_url":"https://api.github.com/users/karusb/subscriptions","organizations_url":"https://api.github.com/users/karusb/orgs","repos_url":"https://api.github.com/users/karusb/repos","events_url":"https://api.github.com/users/karusb/events{/privacy}","received_events_url":"https://api.github.com/users/karusb/received_events","type":"User","site_admin":false},"content_type":"application/x-zip-compressed","state":"uploaded","size":350942,"download_count":49,"created_at":"2021-04-18T12:51:59Z","updated_at":"2021-04-18T12:52:00Z","browser_download_url":"https://github.com/karusb/FLF-FindLargeFolders/releases/download/2.2/BazWareFindLargeFolders-2.2-Installer.zip"},{"url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/releases/assets/35317535","id":35317535,"node_id":"MDEyOlJlbGVhc2VBc3NldDM1MzE3NTM1","name":"BazWareFindLargeFolders-2.2-Portable.zip","label":null,"uploader":{"login":"karusb","id":15250436,"node_id":"MDQ6VXNlcjE1MjUwNDM2","avatar_url":"https://avatars.githubusercontent.com/u/15250436?v=4","gravatar_id":"","url":"https://api.github.com/users/karusb","html_url":"https://github.com/karusb","followers_url":"https://api.github.com/users/karusb/followers","following_url":"https://api.github.com/users/karusb/following{/other_user}","gists_url":"https://api.github.com/users/karusb/gists{/gist_id}","starred_url":"https://api.github.com/users/karusb/starred{/owner}{/repo}","subscriptions_url":"https://api.github.com/users/karusb/subscriptions","organizations_url":"https://api.github.com/users/karusb/orgs","repos_url":"https://api.github.com/users/karusb/repos","events_url":"https://api.github.com/users/karusb/events{/privacy}","received_events_url":"https://api.github.com/users/karusb/received_events","type":"User","site_admin":false},"content_type":"application/x-zip-compressed","state":"uploaded","size":28654,"download_count":113,"created_at":"2021-04-18T12:51:58Z","updated_at":"2021-04-18T12:51:59Z","browser_download_url":"https://github.com/karusb/FLF-FindLargeFolders/releases/download/2.2/BazWareFindLargeFolders-2.2-Portable.zip"}],"tarball_url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/tarball/2.2","zipball_url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/zipball/2.2","body":"- Great dark UI with icon changes \r\n- Able to pick a directory for investigation as well as dropping a folder.\r\n- Control buttons appear after investigation to be able to go back to the previous folder.\r\n- New control bar shows the current directory and can be double-clicked to open the folder in explorer.\r\n- File count in a folder appears next to folder size.\r\n"},{"url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/releases/32394060","assets_url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/releases/32394060/assets","upload_url":"https://uploads.github.com/repos/karusb/FLF-FindLargeFolders/releases/32394060/assets{?name,label}","html_url":"https://github.com/karusb/FLF-FindLargeFolders/releases/tag/1.1","id":32394060,"author":{"login":"karusb","id":15250436,"node_id":"MDQ6VXNlcjE1MjUwNDM2","avatar_url":"https://avatars.githubusercontent.com/u/15250436?v=4","gravatar_id":"","url":"https://api.github.com/users/karusb","html_url":"https://github.com/karusb","followers_url":"https://api.github.com/users/karusb/followers","following_url":"https://api.github.com/users/karusb/following{/other_user}","gists_url":"https://api.github.com/users/karusb/gists{/gist_id}","starred_url":"https://api.github.com/users/karusb/starred{/owner}{/repo}","subscriptions_url":"https://api.github.com/users/karusb/subscriptions","organizations_url":"https://api.github.com/users/karusb/orgs","repos_url":"https://api.github.com/users/karusb/repos","events_url":"https://api.github.com/users/karusb/events{/privacy}","received_events_url":"https://api.github.com/users/karusb/received_events","type":"User","site_admin":false},"node_id":"MDc6UmVsZWFzZTMyMzk0MDYw","tag_name":"1.1","target_commitish":"master","name":"BazWare Find Large Folders v1.1","draft":false,"prerelease":true,"created_at":"2020-10-09T23:27:08Z","published_at":"2020-10-09T23:38:37Z","assets":[{"url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/releases/assets/26779347","id":26779347,"node_id":"MDEyOlJlbGVhc2VBc3NldDI2Nzc5MzQ3","name":"BazWareFLF-1.1.zip","label":null,"uploader":{"login":"karusb","id":15250436,"node_id":"MDQ6VXNlcjE1MjUwNDM2","avatar_url":"https://avatars.githubusercontent.com/u/15250436?v=4","gravatar_id":"","url":"https://api.github.com/users/karusb","html_url":"https://github.com/karusb","followers_url":"https://api.github.com/users/karusb/followers","following_url":"https://api.github.com/users/karusb/following{/other_user}","gists_url":"https://api.github.com/users/karusb/gists{/gist_id}","starred_url":"https://api.github.com/users/karusb/starred{/owner}{/repo}","subscriptions_url":"https://api.github.com/users/karusb/subscriptions","organizations_url":"https://api.github.com/users/karusb/orgs","repos_url":"https://api.github.com/users/karusb/repos","events_url":"https://api.github.com/users/karusb/events{/privacy}","received_events_url":"https://api.github.com/users/karusb/received_events","type":"User","site_admin":false},"content_type":"application/x-zip-compressed","state":"uploaded","size":7087,"download_count":13,"created_at":"2020-10-09T23:36:39Z","updated_at":"2020-10-09T23:36:40Z","browser_download_url":"https://github.com/karusb/FLF-FindLargeFolders/releases/download/1.1/BazWareFLF-1.1.zip"}],"tarball_url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/tarball/1.1","zipball_url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/zipball/1.1","body":"Bug fixes, UI resizes correctly"},{"url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/releases/21861202","assets_url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/releases/21861202/assets","upload_url":"https://uploads.github.com/repos/karusb/FLF-FindLargeFolders/releases/21861202/assets{?name,label}","html_url":"https://github.com/karusb/FLF-FindLargeFolders/releases/tag/1.0-beta","id":21861202,"author":{"login":"karusb","id":15250436,"node_id":"MDQ6VXNlcjE1MjUwNDM2","avatar_url":"https://avatars.githubusercontent.com/u/15250436?v=4","gravatar_id":"","url":"https://api.github.com/users/karusb","html_url":"https://github.com/karusb","followers_url":"https://api.github.com/users/karusb/followers","following_url":"https://api.github.com/users/karusb/following{/other_user}","gists_url":"https://api.github.com/users/karusb/gists{/gist_id}","starred_url":"https://api.github.com/users/karusb/starred{/owner}{/repo}","subscriptions_url":"https://api.github.com/users/karusb/subscriptions","organizations_url":"https://api.github.com/users/karusb/orgs","repos_url":"https://api.github.com/users/karusb/repos","events_url":"https://api.github.com/users/karusb/events{/privacy}","received_events_url":"https://api.github.com/users/karusb/received_events","type":"User","site_admin":false},"node_id":"MDc6UmVsZWFzZTIxODYxMjAy","tag_name":"1.0-beta","target_commitish":"master","name":"BazWare Find Large Folders v1.0","draft":false,"prerelease":true,"created_at":"2019-11-29T16:25:13Z","published_at":"2019-11-29T16:32:00Z","assets":[{"url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/releases/assets/16480968","id":16480968,"node_id":"MDEyOlJlbGVhc2VBc3NldDE2NDgwOTY4","name":"BazWareFLF.zip","label":null,"uploader":{"login":"karusb","id":15250436,"node_id":"MDQ6VXNlcjE1MjUwNDM2","avatar_url":"https://avatars.githubusercontent.com/u/15250436?v=4","gravatar_id":"","url":"https://api.github.com/users/karusb","html_url":"https://github.com/karusb","followers_url":"https://api.github.com/users/karusb/followers","following_url":"https://api.github.com/users/karusb/following{/other_user}","gists_url":"https://api.github.com/users/karusb/gists{/gist_id}","starred_url":"https://api.github.com/users/karusb/starred{/owner}{/repo}","subscriptions_url":"https://api.github.com/users/karusb/subscriptions","organizations_url":"https://api.github.com/users/karusb/orgs","repos_url":"https://api.github.com/users/karusb/repos","events_url":"https://api.github.com/users/karusb/events{/privacy}","received_events_url":"https://api.github.com/users/karusb/received_events","type":"User","site_admin":false},"content_type":"application/x-zip-compressed","state":"uploaded","size":7133,"download_count":10,"created_at":"2019-11-29T16:30:19Z","updated_at":"2019-11-29T16:30:20Z","browser_download_url":"https://github.com/karusb/FLF-FindLargeFolders/releases/download/1.0-beta/BazWareFLF.zip"}],"tarball_url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/tarball/1.0-beta","zipball_url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/zipball/1.0-beta","body":"The initial release of BazWare FLF. \r\nPlease report any bugs or suggestions via issues."}])mixedReleases"};

    std::vector<Release> mixedReleases {{
    "2.2", "BazWare Find Large Folders v2.2", {
    {"https://github.com/karusb/FLF-FindLargeFolders/releases/download/2.2/BazWareFindLargeFolders-2.2-Installer.zip", "BazWareFindLargeFolders-2.2-Installer.zip"},
    {"https://github.com/karusb/FLF-FindLargeFolders/releases/download/2.2/BazWareFindLargeFolders-2.2-Portable.zip", "BazWareFindLargeFolders-2.2-Portable.zip" } },
    {"https://api.github.com/repos/karusb/FLF-FindLargeFolders/tarball/2.2", "2.2.tar.gz",},
    {"https://api.github.com/repos/karusb/FLF-FindLargeFolders/zipball/2.2", "2.2.zip"}, 
    false, false},
    {"1.1", "BazWare Find Large Folders v1.1", {
    {"https://github.com/karusb/FLF-FindLargeFolders/releases/download/1.1/BazWareFLF-1.1.zip", "BazWareFLF-1.1.zip"}},
    {"https://api.github.com/repos/karusb/FLF-FindLargeFolders/tarball/1.1", "1.1.tar.gz",},
    {"https://api.github.com/repos/karusb/FLF-FindLargeFolders/zipball/1.1", "1.1.zip"}, 
    true, false},
    {"1.0-beta", "BazWare Find Large Folders v1.0", {
    {"https://github.com/karusb/FLF-FindLargeFolders/releases/download/1.0-beta/BazWareFLF.zip", "BazWareFLF.zip"}},
    {"https://api.github.com/repos/karusb/FLF-FindLargeFolders/tarball/1.0-beta", "1.0-beta.tar.gz",},
    {"https://api.github.com/repos/karusb/FLF-FindLargeFolders/zipball/1.0-beta", "1.0-beta.zip"}, 
    true, false}};

    std::string latestReleaseResponse = {R"latestRelease({"url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/releases/41605006","assets_url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/releases/41605006/assets","upload_url":"https://uploads.github.com/repos/karusb/FLF-FindLargeFolders/releases/41605006/assets{?name,label}","html_url":"https://github.com/karusb/FLF-FindLargeFolders/releases/tag/2.2","id":41605006,"author":{"login":"karusb","id":15250436,"node_id":"MDQ6VXNlcjE1MjUwNDM2","avatar_url":"https://avatars.githubusercontent.com/u/15250436?v=4","gravatar_id":"","url":"https://api.github.com/users/karusb","html_url":"https://github.com/karusb","followers_url":"https://api.github.com/users/karusb/followers","following_url":"https://api.github.com/users/karusb/following{/other_user}","gists_url":"https://api.github.com/users/karusb/gists{/gist_id}","starred_url":"https://api.github.com/users/karusb/starred{/owner}{/repo}","subscriptions_url":"https://api.github.com/users/karusb/subscriptions","organizations_url":"https://api.github.com/users/karusb/orgs","repos_url":"https://api.github.com/users/karusb/repos","events_url":"https://api.github.com/users/karusb/events{/privacy}","received_events_url":"https://api.github.com/users/karusb/received_events","type":"User","site_admin":false},"node_id":"MDc6UmVsZWFzZTQxNjA1MDA2","tag_name":"2.2","target_commitish":"master","name":"BazWare Find Large Folders v2.2","draft":false,"prerelease":false,"created_at":"2021-04-18T12:50:36Z","published_at":"2021-04-18T12:52:08Z","assets":[{"url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/releases/assets/35317536","id":35317536,"node_id":"MDEyOlJlbGVhc2VBc3NldDM1MzE3NTM2","name":"BazWareFindLargeFolders-2.2-Installer.zip","label":null,"uploader":{"login":"karusb","id":15250436,"node_id":"MDQ6VXNlcjE1MjUwNDM2","avatar_url":"https://avatars.githubusercontent.com/u/15250436?v=4","gravatar_id":"","url":"https://api.github.com/users/karusb","html_url":"https://github.com/karusb","followers_url":"https://api.github.com/users/karusb/followers","following_url":"https://api.github.com/users/karusb/following{/other_user}","gists_url":"https://api.github.com/users/karusb/gists{/gist_id}","starred_url":"https://api.github.com/users/karusb/starred{/owner}{/repo}","subscriptions_url":"https://api.github.com/users/karusb/subscriptions","organizations_url":"https://api.github.com/users/karusb/orgs","repos_url":"https://api.github.com/users/karusb/repos","events_url":"https://api.github.com/users/karusb/events{/privacy}","received_events_url":"https://api.github.com/users/karusb/received_events","type":"User","site_admin":false},"content_type":"application/x-zip-compressed","state":"uploaded","size":350942,"download_count":49,"created_at":"2021-04-18T12:51:59Z","updated_at":"2021-04-18T12:52:00Z","browser_download_url":"https://github.com/karusb/FLF-FindLargeFolders/releases/download/2.2/BazWareFindLargeFolders-2.2-Installer.zip"},{"url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/releases/assets/35317535","id":35317535,"node_id":"MDEyOlJlbGVhc2VBc3NldDM1MzE3NTM1","name":"BazWareFindLargeFolders-2.2-Portable.zip","label":null,"uploader":{"login":"karusb","id":15250436,"node_id":"MDQ6VXNlcjE1MjUwNDM2","avatar_url":"https://avatars.githubusercontent.com/u/15250436?v=4","gravatar_id":"","url":"https://api.github.com/users/karusb","html_url":"https://github.com/karusb","followers_url":"https://api.github.com/users/karusb/followers","following_url":"https://api.github.com/users/karusb/following{/other_user}","gists_url":"https://api.github.com/users/karusb/gists{/gist_id}","starred_url":"https://api.github.com/users/karusb/starred{/owner}{/repo}","subscriptions_url":"https://api.github.com/users/karusb/subscriptions","organizations_url":"https://api.github.com/users/karusb/orgs","repos_url":"https://api.github.com/users/karusb/repos","events_url":"https://api.github.com/users/karusb/events{/privacy}","received_events_url":"https://api.github.com/users/karusb/received_events","type":"User","site_admin":false},"content_type":"application/x-zip-compressed","state":"uploaded","size":28654,"download_count":100,"created_at":"2021-04-18T12:51:58Z","updated_at":"2021-04-18T12:51:59Z","browser_download_url":"https://github.com/karusb/FLF-FindLargeFolders/releases/download/2.2/BazWareFindLargeFolders-2.2-Portable.zip"}],"tarball_url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/tarball/2.2","zipball_url":"https://api.github.com/repos/karusb/FLF-FindLargeFolders/zipball/2.2","body":"- Great dark UI with icon changes \r\n- Able to pick a directory for investigation as well as dropping a folder.\r\n- Control buttons appear after investigation to be able to go back to the previous folder.\r\n- New control bar shows the current directory and can be double-clicked to open the folder in explorer.\r\n- File count in a folder appears next to folder size.\r\n"})latestRelease"};

    Release latestRelease {"2.2", "BazWare Find Large Folders v2.2", {
    {"https://github.com/karusb/FLF-FindLargeFolders/releases/download/2.2/BazWareFindLargeFolders-2.2-Installer.zip", "BazWareFindLargeFolders-2.2-Installer.zip"},
    {"https://github.com/karusb/FLF-FindLargeFolders/releases/download/2.2/BazWareFindLargeFolders-2.2-Portable.zip", "BazWareFindLargeFolders-2.2-Portable.zip" } }, {"https://api.github.com/repos/karusb/FLF-FindLargeFolders/tarball/2.2", "2.2.tar.gz",}, {"https://api.github.com/repos/karusb/FLF-FindLargeFolders/zipball/2.2", "2.2.zip"}, false, false};

};

#endif
