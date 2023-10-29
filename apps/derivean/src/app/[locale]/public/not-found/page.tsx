import {
    Page,
    Status
} from "@use-pico2/ui";

export default function Custom404() {
    return <Page>
        <Status
            header={"4o4"}
            title={"error.404.title"}
            message={"error.404.message"}
        />
    </Page>;
};
