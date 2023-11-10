import {Text}           from "@use-pico/ui";
import {type ReactNode} from "react";

export const withRichComponents = <TExtra extends Record<string, ReactNode>>(extra?: TExtra) => ({
    b:    <b/>,
    p:    <p/>,
    i:    <i/>,
    mark: <Text size={"xl"}/>,
    ...extra,
}) as const;
