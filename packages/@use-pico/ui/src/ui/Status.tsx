import {
    type FC,
    type PropsWithChildren,
    type ReactNode
}                  from "react";
import {Container} from "./Container";
import {Group}     from "./Group";
import classes     from "./Status.module.css";
import {Text}      from "./Text";
import {Title}     from "./Title";

export namespace Status {
    export type Props = PropsWithChildren<{
        label?: {
            header?: ReactNode;
            title?: ReactNode;
            message?: ReactNode;
        }
    }>;
}

export const Status: FC<Status.Props> = (
    {
        label,
        children
    }) => {
    return <Container className={classes.root}>
        {label?.header && <div className={classes.label}>
            {label?.header}
        </div>}
        {label?.title && <Title className={classes.title}>
            {label?.title}
        </Title>}
        {label?.message && <Text size={"lg"} className={classes.description}>
            {label?.message}
        </Text>}
        <Group justify={"center"}>
            {children}
        </Group>
    </Container>;
};
