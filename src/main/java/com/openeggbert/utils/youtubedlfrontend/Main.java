///////////////////////////////////////////////////////////////////////////////////////////////
// youtubedl-frontend: Tool generating html pages for Archive Box.
// Copyright (C) 2024 the original author or authors.
//
// This program is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see 
// <https://www.gnu.org/licenses/> or write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
///////////////////////////////////////////////////////////////////////////////////////////////
package com.openeggbert.utils.youtubedlfrontend;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import static com.openeggbert.utils.youtubedlfrontend.Args.TWO_DASHES;

/**
 * @author <a href="mailto:mail@robertvokac.com">Robert Vokac</a>
 * @since 0.0.0
 */
public class Main {

    private static int iii = 0;
    private static int internalStaticVariableVideoNumberPerRow = 0;

    public static int THUMBNAIL_WIDTH = 250;

    public static void main(String[] args) throws IOException, InterruptedException {
        System.out.println("youtubedlfrontend - HTML generator\n");

        if (args.length < 1) {
            //System.err.println("At least one argument is expected, but the count of arguments is: " + args.length + ".");
            String argsS = "/rv/big/foreign-blupi-videos-on-youtube --video_ 5rGd2VQz3mo --always-generate-metadata 1"
                    + " --always-generate-html-files 1 --videos-per-row 4 --thumbnail-links-to-youtube 0"
                    + " --thumbnail-as-base64 1"
                    + " --channel_ UCqBpgfXap7cZOYkAC34u8Lg ";
            args = argsS.split(" ");
            //System.exit(1);
        }
        Args argsInstance = new Args(args);
        System.out.println(argsInstance.toString());
        String workingDirectory = args.length > 0 && !args[0].startsWith(TWO_DASHES) ? args[0] : new File(".").getAbsolutePath();

        File archiveBoxRootDirectory = new File(workingDirectory);
        File archiveBoxArchiveDirectory = new File(archiveBoxRootDirectory, "archive");

        List<YoutubeVideo> youtubeVideos = YoutubeVideo.loadYoutubeVideos(archiveBoxArchiveDirectory, argsInstance);
        Map<String, String> channelUrls = new HashMap<>();
        List<String> channels = new ArrayList<>();
        youtubeVideos.stream().forEach(v -> {
            final String channelName = v.getChannelName();

            if (channelName != null && !channelUrls.containsKey(v.getChannelName())) {
                channelUrls.put(channelName, v.getChannelUrl());
                channels.add(channelName);
            }
        });
        Collections.sort(channels, (String o1, String o2) -> o1.toLowerCase().compareTo(o2.toLowerCase()));

        File videosHtmlFile = new File(archiveBoxRootDirectory, "videos.html");
        File videosDirectory = new File(archiveBoxRootDirectory, "videos");
        File channelsDirectory = new File(archiveBoxRootDirectory, "channels");
        if (!videosDirectory.exists()) {
            videosDirectory.mkdir();
        }
        if (!channelsDirectory.exists()) {
            channelsDirectory.mkdir();
        }
        channels.stream().forEach(c -> {
            StringBuilder oneChannelStringBuilder = createChannelHtml(c, channels, argsInstance, channelUrls, youtubeVideos, archiveBoxRootDirectory, videosDirectory, archiveBoxArchiveDirectory);
            Utils.writeTextToFile(oneChannelStringBuilder.toString(), new File(channelsDirectory, channelUrls.get(c).split("/channel/")[1] + ".html"));
        });
        StringBuilder oneChannelStringBuilder = createChannelHtml(null, channels, argsInstance, channelUrls, youtubeVideos, archiveBoxRootDirectory, videosDirectory, archiveBoxArchiveDirectory);
        Utils.writeTextToFile(oneChannelStringBuilder.toString(), videosHtmlFile);

        System.out.println("[Warning] Snapshots without videos:");
        YoutubeVideo.missingYoutubeVideos.forEach(s -> System.out.println(s));
        System.out.println("Total duration: " + ((int) ((((double) YoutubeVideo.totalDurationInMilliseconds) / 1000d / 60d / 60d))) + " hours");
        youtubeVideos.stream().sorted((YoutubeVideo o1, YoutubeVideo o2) -> Long.valueOf(o1.getVideoDurationInMilliseconds()).compareTo(Long.valueOf(o2.getVideoDurationInMilliseconds())))
                .forEach(y -> {
                    System.out.println(y.getVideoDurationInMinutes() + " = minutes \t" + "https://youtube.com/watch?v=" + y.getId() + "\t" + y.getTitle());
                });
        System.out.println("\n\n\n\n");
        youtubeVideos.stream().sorted((YoutubeVideo o1, YoutubeVideo o2) -> Long.valueOf(o1.getVideoFileSizeInBytes()).compareTo(Long.valueOf(o2.getVideoFileSizeInBytes())))
                .forEach(y -> {
                    System.out.println(y.getVideoFileSizeInMegaBytes() + " MB \t" + "https://youtube.com/watch?v=" + y.getId() + "\t" + y.getTitle());
                });
    }

    private static StringBuilder createChannelHtml(String wantedChannelName, List<String> channels, Args argsInstance, Map<String, String> channelUrls, List<YoutubeVideo> youtubeVideos, File archiveBoxRootDirectory, File videosDirectory, File archiveBoxArchiveDirectory) {
        StringBuilder oneChannelStringBuilder = new StringBuilder();
        oneChannelStringBuilder.append("""
                                                <!DOCTYPE html>
                                               <html>
                                               <head>
                                               <meta charset="UTF-8"> 
                                               <link rel="icon" type="image/x-icon" href="favicon.ico" sizes="16x16">
                                               <title>Youtube videos</title>
                                                                             <!-- Generated by: https://code.openeggbert.org/openeggbert/youtubedl-frontend -->
                                                                             <style>
                                               body {padding:20px;}
                                               * {
                                                 font-family:Arial;
                                               }
                                               .videos {
                                                 /*box-sizing: border-box;*/
                                               }
                                               .box {
                                                 /*float: left;
                                                 width: 20.0%;*/
                                                 padding: 10px;
                                               }
                                               </style>
                                               </head>
                                               <body>
                                               """);
        channels.stream().filter(c -> wantedChannelName == null ? true : c.equals(wantedChannelName)).forEach(channel -> {
            oneChannelStringBuilder.append("<h1>").append(channel).append("</h1>\n");
            oneChannelStringBuilder.append("<div style=\"max-width:").append((Main.THUMBNAIL_WIDTH + 20) * argsInstance.getInteger(ArgType.VIDEOS_PER_ROW).get()).append("px\">");

            oneChannelStringBuilder.append("<a target=\"_blank\" href =\"channels/").append(channelUrls.get(channel).split("/channel/")[1]).append(".html").append("\">").append("Videos").append("</a>");
            oneChannelStringBuilder.append("&nbsp;&nbsp;&nbsp;( <a href =\"").append(channelUrls.get(channel)).append("\">").append(channelUrls.get(channel)).append("</a> )");

            if (wantedChannelName != null) {
                oneChannelStringBuilder.append("<div class=\"videos\">");
                iii = 0;
                internalStaticVariableVideoNumberPerRow = 0;
                oneChannelStringBuilder.append("<table>\n");
                long countOfVideosInChannel = youtubeVideos.stream().filter(v -> channel.equals(v.getChannelName())).count();
                youtubeVideos.stream().filter(v -> channel.equals(v.getChannelName())).forEach(youtubeVideo -> {
                    iii++;
                    if (internalStaticVariableVideoNumberPerRow == 0) {
                        oneChannelStringBuilder.append("<tr>");
                    }
                    internalStaticVariableVideoNumberPerRow++;
                    oneChannelStringBuilder.append("<td><div class=\"box\"><table style=\"margin:5px;max-width:")
                            .append(THUMBNAIL_WIDTH)
                            .append("px;\">\n<tr><td><a href=\"");
                    if (argsInstance.getBoolean(ArgType.THUMBNAIL_LINKS_TO_YOUTUBE).get()) {
                        oneChannelStringBuilder.append("https://www.youtube.com/watch?v=").append(youtubeVideo.getId());
                    } else {
                        oneChannelStringBuilder.append("../videos/" + youtubeVideo.getId() + ".html");
                    }
                    oneChannelStringBuilder.append("\" target=\"_blank\"><img src=\"");
                    String thumbnailPath = new StringBuilder()
                            .append("archive/")
                            .append(youtubeVideo.getSnapshot())
                            .append("/media/mini-thumbnail.")
                            .append(youtubeVideo.getMiniThumbnailFormat()).toString();
                    if (argsInstance.getBoolean(ArgType.THUMBNAIL_AS_BASE64).get()) {
                        try {
                            byte[] bytes = Files.readAllBytes(new File(archiveBoxRootDirectory + "/" + thumbnailPath).toPath());
                            System.out.println("###=" + archiveBoxRootDirectory + "/" + thumbnailPath);
                            ByteArrayInputStream bais = new ByteArrayInputStream(bytes);
                            try {
                                bytes = Utils.resizeImage(bais, 25, (int) (9d / 16d * 25d), youtubeVideo.getThumbnailFormat());
                            } catch (Exception e) {
                                //bytes = Utils.resizeImage(bais, 125, (int) (9d / 16d * 125d), "webp");
                            }

                            String bytesS = "data:image/jpg;base64, " + com.openeggbert.powerframework.io.bit.base64.Base64Coder.encode(bytes);
                            oneChannelStringBuilder.append(bytesS);
                        } catch (IOException ex) {
                            throw new YoutubedlFrontendException(ex.getMessage());
                        }
                    } else {
                        oneChannelStringBuilder.append("../" + thumbnailPath);
                    }
                    oneChannelStringBuilder.append("\" width=\"")
                            .append(THUMBNAIL_WIDTH)
                            .append("\"></a></td></tr>\n");
                    oneChannelStringBuilder.append("<tr><td><b style=\"font-size:90%;\">").append(youtubeVideo.getTitle()).append("</b></td></tr>\n");
                    String uploadDate = youtubeVideo.getUploadDate();
                    uploadDate = uploadDate.substring(0, 4) + "-" + uploadDate.substring(4, 6) + "-" + uploadDate.substring(6, 8);
                    oneChannelStringBuilder.append("<tr><td style=\"font-size:80%;color:grey;\">").append(uploadDate).append(" •︎ ").append(youtubeVideo.getVideoDuration())
                            .append(" •︎ ")
                            .append("#").append(iii)
                            .append("</td></tr>\n");
                    youtubeVideo.setNumber(iii);
                    oneChannelStringBuilder.append("</table></div></td>\n");
                    if (internalStaticVariableVideoNumberPerRow == argsInstance.getInteger(ArgType.VIDEOS_PER_ROW).get()) {
                        oneChannelStringBuilder.append("<tr>");
                        internalStaticVariableVideoNumberPerRow = 0;
                    }
                    File videoHtmlFile = new File(videosDirectory, youtubeVideo.getId() + ".html");
                    if (!videoHtmlFile.exists() || argsInstance.getBoolean(ArgType.ALWAYS_GENERATE_HTML_FILES).get()) {

                        {
                            String singleVideo = new YoutubeVideoHtml(youtubeVideo, archiveBoxRootDirectory, archiveBoxArchiveDirectory, countOfVideosInChannel).toString();
                            Utils.writeTextToFile(singleVideo, videoHtmlFile);
                            processedVideos++;
                            System.out.println("Processed " + processedVideos + " from " + archiveBoxArchiveDirectory.list().length);
                        }
                    }
                });
                if (internalStaticVariableVideoNumberPerRow < argsInstance.getInteger(ArgType.VIDEOS_PER_ROW).get()) {
                    oneChannelStringBuilder.append("<tr>");
                }
                oneChannelStringBuilder.append("</table>\n");
                oneChannelStringBuilder.append("</div>");
            }
            oneChannelStringBuilder.append("</div>");
        });
        oneChannelStringBuilder.append("""
                                                                                                           </body>
                                               </html>
                                               """);
        return oneChannelStringBuilder;
    }

    private static int processedVideos = 0;

}
