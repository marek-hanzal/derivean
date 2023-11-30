import {
    type FC,
    type ReactNode
} from "react";

export namespace Preview {
    export interface Props {
        items: Item[];
    }

    export interface Item {
        key: string;
        label: ReactNode;
        value: ReactNode;
    }
}

export const Preview: FC<Preview.Props> = () => {
    return "Preview";
};
