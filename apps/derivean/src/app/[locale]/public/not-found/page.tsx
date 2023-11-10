import {t} from "@use-pico/i18n";
import {
    Page,
    Status
}          from "@use-pico/ui";

export default function Custom404() {
    return <Page>
        <Status
            text={{
                header: t()`4o4`,
                title: t()`You found a place where nothing is hidden.`,
                message: t()`Here or there, you see nothing interesting. Try another page.`,
            }}
        />
    </Page>;
};
