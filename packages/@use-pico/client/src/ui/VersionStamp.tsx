import {type FC} from "react";
import {Text}    from "./Text";

export namespace VersionStamp {
    export interface Props {
    }
}

export const VersionStamp: FC<VersionStamp.Props> = () => {
    return <Text c={"dimmed"}>{process.env.NEXT_PUBLIC_VERSION}</Text>;
};
