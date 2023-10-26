import {
    Page,
    Status
} from "@use-pico/ui";

export default function Custom404() {
    return <Page
        title={"public.404"}
    >
        <Status
            header={"4o4"}
            title={"404.title"}
            message={"404.message"}
        />
    </Page>;
};
