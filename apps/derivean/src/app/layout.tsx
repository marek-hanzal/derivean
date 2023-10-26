import {ColorSchemeScript}      from "@mantine/core";
import "@mantine/core/styles.css";
import "@mantine/dropzone/styles.css";
import "@mantine/notifications/styles.css";
import "@mantine/spotlight/styles.css";
import {type Metadata}          from "next";
import {type PropsWithChildren} from "react";

export const metadata: Metadata = {
    title: "The Legacy of DeRivean",
};

export default function Layout({children}: PropsWithChildren) {
    return <html>
        <head>
            <ColorSchemeScript/>
        </head>
        <body>
            {children}
        </body>
    </html>;
}
